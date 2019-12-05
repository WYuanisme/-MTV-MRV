//--------------------------------------------------------------------
// Code by WangYuan; Date:2019/11/12
// 输入：MTV、MRV文本文件
// 输出：短路、呆滞、强驱动、未知故障，误判、混淆网络，检测率、隔离率
//--------------------------------------------------------------------


#include "analyze_mtv.h"

#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QRegExp>
#include <QChar>
#include <math.h>



// (1)读txt，把网络名和STV，存进容器MTV。把网络名和SRV，存进容器MRV
// (2)找出所有与STV不同的SRV，修改flag。flag第i个元素对应MRV中的第i行SRV,SRV不等于STV则该元素值为1，否则为0。（后续若该行已经处理过，元素i值为2，此步并不处理）
// (3)遍历flag，若元素==1，找到对应MRV行，分析故障。
// (4)分析征兆误判率和征兆混效率。
Analyze_MTV::Analyze_MTV(const QString &MTV_file_path,const QString &MRV_file_path)
{

    // (1)读txt，把网络名和STV，存进容器MTV。把网络名和SRV，存进容器MRV
    get_file_content(MTV_file_path,MTV);
    get_file_content(MRV_file_path,MRV);
    // (2)找出所有与STV不同的SRV，flag第i个元素对应MRV中的第i行SRV,SRV不等于STV则该元素值为1，否则为0。若该行已经处理过，元素i值为2
    get_flag_srv_diff_to_stv();     // 找出所有与STV不同的SRV，修改flag
    // (3)遍历flag，若元素==1，找到对应MRV行，分析故障。
    analyze_fault();
    // (4)分析征兆误判率和征兆混效率。
    analyze_rate();
}

// =================================私有函数：输入文本文件名，把文件内容存进容器================================================
bool Analyze_MTV::get_file_content(const QString &file_path,QVector<QVector<QString> > &matrix)
{
    // QFile参数就是读取文件路径
    QFile file(file_path);
    file.open(QIODevice::ReadOnly);  // 一定要加这一句，调试了好久发现qstr_all_file是空的，就是因为没加这一句文件根本没打开
    QTextStream  qstr_all_file(&file);
    while( !qstr_all_file.atEnd())
    {
        QString qstr_temp = qstr_all_file.readLine();   // 此时读入的一行不包括换行符

        QString temp_name;            // 临时的字符串存网络名
        QString temp_stv_srv;         // 存网络名后的STV或SRV
        QVector<QString> temp_row;    // 保存上面两个字符串
        QString::iterator it_begin=qstr_temp.begin();
        QString::iterator it_end=qstr_temp.end();
        auto it = it_begin;
        while (it!=it_end)
        {
            if(*it!='\x20')      // 如果没有遇到第一个空格，则是网络名，一个字符一个字符存进temp_name
            {
                temp_name.push_back(*it);
            }
            else                 // 遇到第一个空格后，先把网络名存进临时一行
            {
                temp_row.push_back(temp_name);
                ++it;
                break;
                // 跳出循环时，迭代器指向stv或srv的第一个字母
            }
            ++it;
        }
        // 从stv或srv的第一个字母一直到结尾，全部拷贝
        int index_begin = it-it_begin;
        temp_stv_srv = qstr_temp.mid(index_begin);
        // 把01串里面的空格去掉
        QRegExp reg("\x20");  // 定义正则表达式，匹配所有空格
        temp_stv_srv = temp_stv_srv.replace(reg,"");
        // 把测试向量存进临时的一行
        temp_row.push_back(temp_stv_srv);
        // 把临时的一行，存进测试矩阵
        matrix.push_back(temp_row);
    }
    file.close();
    // 若修改后的矩阵为空，返回0，表示操作失败，否则返回1
    if(matrix.empty())
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// =================================私有函数：找出所有与STV不同的SRV，修改flag============================================
bool Analyze_MTV::get_flag_srv_diff_to_stv()
{
    if(MRV.isEmpty() || MTV.isEmpty())
    {
        return 0;   // 如果两个矩阵为空，后续不同操作了，返回0，表示该步操作失败
    }
    else
    {
        int ii=0;
        for(const auto &i:MRV)
        {
            int jj=0;
            for(const auto &j:i)
            {
                if(jj)   // 如果jj==1，表示j为矩阵第1列，里面保存的是SRV
                {
                    if(j==MTV[ii][jj])  // 如果SRV==STV，flag设为0
                    {
                        flag.append(0);
                    }
                    else                // 如果SRV!=STV，flag设为1
                    {
                        flag.append(1);
                    }
                }
                ++jj;
            }
            ++ii;
        }
        return 1;
    }
}

// =================================遍历flag，若元素==1，找到对应MRV行，分析故障===========================================
// (1)全0，呆滞0故障；全1，呆滞1故障
// (2)遍历flag≠2的SRV，找到与SRV_i相等的SRV_j、k、l、m……，判断相等的SRV个数 equal_num
// (3)equal_num=1，未知故障，flag置2
// (4)equal_num=2，若flag_j==0，SRV_j征兆误判；若flag_j==1，再判断是W-O还是W-A短路。
// (5)equal_num=3，若flag_j==0，SRV_j征兆误判；若flag_k==0，SRV_k征兆误判；若flag_j、flag_k==1，再判断是W-O还是W-A短路。
// (6)equal_num=4，排列组合判断网络间短路情况。目前懒得写，就归在征兆混淆里面。
// (7)equal_num>4，征兆混淆。
void Analyze_MTV::analyze_fault()
{
    int ii=0;
    for(auto &i:flag) // 不用const，因为处理过的1要设为2
    {
        if(i==1)      // falg==1有故障
        {
            // (1)全0，呆滞0故障；全1，呆滞1故障
            int have_zero = 0;
            int have_one = 0;
            for(const auto &char_of_srv : MRV[ii][1])
            {
                if(char_of_srv == '0')
                {
                    have_zero = 1;
                }
                if(char_of_srv == '1')
                {
                    have_one = 1;
                }
            }
            if(have_zero == 1 && have_one == 0)         // 如果SRV全0，则have_zero == 1且have_one == 0
            {
                // 保存呆滞0故障网络，每行，第0个元素为网络名，一为STV，二为SRV
                QVector<QString> temp_qstr;         // 临时行
                temp_qstr.push_back(MTV[ii][0]);    // 第0个元素为网络名
                temp_qstr.push_back(MTV[ii][1]);
                temp_qstr.push_back(MRV[ii][1]);
                s_a_0_fault.push_back(temp_qstr);   // 将这一行存进容器
                i = 2;      // 修改flag的值，表示这一行MRV的故障已经判断了
            }
            else if(have_zero == 0 && have_one == 1)     // 如果SRV全1，则have_zero == 0且have_one == 1
            {
                // 保存呆滞1故障网络，每行，第0个元素为网络名，一为STV，二为SRV
                QVector<QString> temp_qstr;         // 临时行
                temp_qstr.push_back(MTV[ii][0]);    // 第0个元素为网络名
                temp_qstr.push_back(MTV[ii][1]);
                temp_qstr.push_back(MRV[ii][1]);
                s_a_1_fault.push_back(temp_qstr);   // 将这一行存进容器
                i = 2;      // 修改flag的值，表示这一行MRV的故障已经判断了
            }
            else    // have_zero == 1且have_one == 1，SRV有0有1。  have_zero == 0且have_one == 0，这种情况错误！
            {
                // (2)遍历flag≠2的SRV，找到与SRV_i相等的SRV_j、k、l、m……，判断相等的SRV个数 equal_num
                QVector<int> equal_srv_index;       // 保存所有与SRV_i相等的SRV_j、k、l、m……的下标，当然SRV_i的下标也要保存
                equal_srv_index.push_back(ii);
                int mm = 0;
                for(auto &m:flag)
                {
                    if(m!=2)    // 如果flag!=2，表示该SRV未被处理过
                    {
                        if( MRV[ii][1]==MRV[mm][1] && ii!=mm )    // 判断与SRV_i是否相等
                        {
                            equal_srv_index.push_back(mm);      // 相等就保存当前SRV下标
                        }
                    }
                    ++mm;
                }
                int equal_num = equal_srv_index.size();
                switch (equal_num)
                {
                {case 1:
                        // (3)equal_num=1，未知故障，flag置2
                        for(auto &n:equal_srv_index)
                        {
                            unknow_fault.push_back(MRV[n]);
                            flag[n]=2;
                        }
                        break;
                }
                {case 2:
                        // (4)equal_num=2，若flag_j==0，SRV_j强驱动故障；若flag_j==1，再判断是W-O还是W-A短路。flag置2
                        int jj = equal_srv_index[1];
                        QString temp_srv1 = bit_or(MTV[ii][1],MTV[jj][1]);   // 调用按位或函数，得到按位或结果
                        QString temp_srv2 = bit_and(MTV[ii][1],MTV[jj][1]);  // 按位与函数
                        // W-O短路
                        if( temp_srv1 ==  MRV[ii][1])
                        {
                            // 保存线或短路故障，一个矩阵存某几个短在一起的网络，每行一个网络，第0个元素为网络名，一STV，二SRV。
                            QVector<QVector<QString> > temp_matrix;
                            QVector<QString> temp_row;
                            for(const auto &index:equal_srv_index)
                            {
                                temp_row.push_back(MTV[index][0]);    // 第index个元素为网络名，
                                temp_row.push_back(MTV[index][1]);    // 一STV
                                temp_row.push_back(MRV[index][1]);    // 二SRV
                                temp_matrix.push_back(temp_row);
                                temp_row.clear();
                            }
                            w_o_fault.push_back(temp_matrix);
                        }
                        // W-A短路
                        else if( temp_srv2 == MRV[ii][1])
                        {
                            // 保存线与短路故障，一个矩阵存某几个短在一起的网络，每行一个网络，第0个元素为网络名，一STV，二SRV。
                            QVector<QVector<QString> > temp_matrix;
                            QVector<QString> temp_row;
                            for(const auto &index:equal_srv_index)
                            {
                                temp_row.push_back(MTV[index][0]);    // 第index个元素为网络名，
                                temp_row.push_back(MTV[index][1]);    // 一STV
                                temp_row.push_back(MRV[index][1]);    // 二SRV
                                temp_matrix.push_back(temp_row);
                                temp_row.clear();
                            }
                            w_a_fault.push_back(temp_matrix);
                        }
                        // 强驱动短路故障
                        else
                        {
                            // 保存短路故障，一个矩阵存某几个短在一起的网络，每行一个网络，第0个元素为网络名，一STV，二SRV。
                            QVector<QVector<QString> > temp_matrix;
                            QVector<QString> temp_row;
                            for(const auto &index:equal_srv_index)
                            {
                                temp_row.push_back(MTV[index][0]);    // 第index个元素为网络名，
                                temp_row.push_back(MTV[index][1]);    // 一STV
                                temp_row.push_back(MRV[index][1]);    // 二SRV
                                temp_matrix.push_back(temp_row);
                                temp_row.clear();
                            }
                            unknow_short_fault.push_back(temp_matrix);   // 此时应该是强驱动短路故障
                        }
                        // 把处理过的SRV的flag置2
                        for(auto &index:equal_srv_index)
                        {
                            flag[index]=2;
                        }
                        break;
                }
                {case 3:
                        // (5)equal_num=3，若flag_j==0，SRV_j征兆误判；若flag_k==0，SRV_k征兆误判；若flag_j、flag_k==1，再判断是W-O还是W-A短路。flag置2
                        int jj = equal_srv_index[1];
                        int kk = equal_srv_index[2];
                        // SRV_j征兆误判
                        if(flag[jj]==0 && flag[kk]!=0)
                        {                            
                            // 保存征兆误判的网络，一个矩阵存误判的网络和其发生误判的其他网络。每行，第0个元素为网络名，一STV，二SRV
                            QVector<QVector<QString> > temp_aliasing;
                            QVector<QString> temp_row;
                            temp_row.push_back(MTV[jj][0]);    // 第0个元素为网络名，
                            temp_row.push_back(MTV[jj][1]);    // 一STV
                            temp_row.push_back(MRV[jj][1]);    // 二SRV
                            temp_aliasing.push_back(temp_row);
                            temp_row.clear();

                            temp_row.push_back(MTV[ii][0]);    // 第1个元素为网络名，
                            temp_row.push_back(MTV[ii][1]);    // 一STV
                            temp_row.push_back(MRV[ii][1]);    // 二SRV
                            temp_aliasing.push_back(temp_row);
                            temp_row.clear();

                            temp_row.push_back(MTV[kk][0]);    // 第2个元素为网络名，
                            temp_row.push_back(MTV[kk][1]);    // 一STV
                            temp_row.push_back(MRV[kk][1]);    // 二SRV
                            temp_aliasing.push_back(temp_row);
                            aliasing.push_back(temp_aliasing);
                        }
                         // SRV_k征兆误判
                        else if(flag[jj]!=0 && flag[kk]==0)
                        {                           
                            // 保存征兆误判的网络，一个矩阵存误判的网络和其发生误判的其他网络。每行，第0个元素为网络名，一STV，二SRV
                            QVector<QVector<QString> > temp_aliasing;
                            QVector<QString> temp_row;
                            temp_row.push_back(MTV[kk][0]);    // 第0个元素为网络名，
                            temp_row.push_back(MTV[kk][1]);    // 一STV
                            temp_row.push_back(MRV[kk][1]);    // 二SRV
                            temp_aliasing.push_back(temp_row);
                            temp_row.clear();

                            temp_row.push_back(MTV[ii][0]);    // 第1个元素为网络名，
                            temp_row.push_back(MTV[ii][1]);    // 一STV
                            temp_row.push_back(MRV[ii][1]);    // 二SRV
                            temp_aliasing.push_back(temp_row);
                            temp_row.clear();

                            temp_row.push_back(MTV[jj][0]);    // 第2个元素为网络名，
                            temp_row.push_back(MTV[jj][1]);    // 一STV
                            temp_row.push_back(MRV[jj][1]);    // 二SRV
                            temp_aliasing.push_back(temp_row);
                            aliasing.push_back(temp_aliasing);
                        }
                        // 判断是W-O还是W-A短路
                        else
                        {                            
                            QString temp_srv11 = bit_or(MTV[ii][1],MTV[jj][1]);   // 调用按位或函数，得到按位或结果
                            QString temp_srv1 = bit_or(temp_srv11,MTV[kk][1]);    // 得到3个字符串按位或的结果， 就调用两边函数
                            QString temp_srv21 = bit_and(MTV[ii][1],MTV[jj][1]);  // 按位与函数
                            QString temp_srv2 = bit_and(temp_srv21,MTV[kk][1]);
                            // W-O短路
                            if(MRV[ii][1] == temp_srv1)
                            {                                
                                // 保存网络，一个矩阵存误判的网络和其发生误判的其他网络。每行，第0个元素为网络名，一STV，二SRV
                                QVector<QVector<QString> > temp_matrix;
                                QVector<QString> temp_row;
                                for(const auto &index:equal_srv_index)
                                {
                                    temp_row.push_back(MTV[index][0]);    // 第index个元素为网络名，
                                    temp_row.push_back(MTV[index][1]);    // 一STV
                                    temp_row.push_back(MRV[index][1]);    // 二SRV
                                    temp_matrix.push_back(temp_row);
                                    temp_row.clear();
                                }
                                w_o_fault.push_back(temp_matrix);
                            }
                            // W-A短路
                            else if(MRV[ii][1] == temp_srv2)
                            {                                
                                // 保存网络，一个矩阵存误判的网络和其发生误判的其他网络。每行，第0个元素为网络名，一STV，二SRV
                                QVector<QVector<QString> > temp_matrix;
                                QVector<QString> temp_row;
                                for(const auto &index:equal_srv_index)
                                {
                                    temp_row.push_back(MTV[index][0]);    // 第index个元素为网络名，
                                    temp_row.push_back(MTV[index][1]);    // 一STV
                                    temp_row.push_back(MRV[index][1]);    // 二SRV
                                    temp_matrix.push_back(temp_row);
                                    temp_row.clear();
                                }
                                w_a_fault.push_back(temp_matrix);
                            }
                            // 不晓得是什么样的短路
                            else
                            {                                
                                // 保存网络，一个矩阵存误判的网络和其发生误判的其他网络。每行，第0个元素为网络名，一STV，二SRV
                                QVector<QVector<QString> > temp_matrix;
                                QVector<QString> temp_row;
                                for(const auto &index:equal_srv_index)
                                {
                                    temp_row.push_back(MTV[index][0]);    // 第index个元素为网络名，
                                    temp_row.push_back(MTV[index][1]);    // 一STV
                                    temp_row.push_back(MRV[index][1]);    // 二SRV
                                    temp_matrix.push_back(temp_row);
                                    temp_row.clear();
                                }
                                unknow_short_fault.push_back(temp_matrix);
                            }
                        }

                        // 把处理过的SRV的flag置2
                        for(auto &index:equal_srv_index)
                        {
                            flag[index]=2;
                        }
                        break;
                }
                {case 4:
                        // (6)equal_num=4，排列组合判断网络间短路情况。flag置2
                        // 目前懒得写这个情况，分情况太多了，就先按征兆混淆判断
                        QVector<QVector<QString> > temp_matrix;
                        QVector<QString> temp_row;
                        for(const auto &index:equal_srv_index)
                        {
                            temp_row.push_back(MTV[index][0]);    // 第index个元素为网络名，
                            temp_row.push_back(MTV[index][1]);    // 一STV
                            temp_row.push_back(MRV[index][1]);    // 二SRV
                            temp_matrix.push_back(temp_row);
                            temp_row.clear();
                        }
                        confounding.push_back(temp_matrix);
                        // 把处理过的SRV的flag置2
                        for(auto &index:equal_srv_index)
                        {
                            flag[index]=2;
                        }
                        break;
                }
                {default:
                        // (7)equal_num>4，征兆混淆。flag置2
                        QVector<QVector<QString> > temp_matrix;
                        QVector<QString> temp_row;
                        for(const auto &index:equal_srv_index)
                        {
                            temp_row.push_back(MTV[index][0]);    // 第index个元素为网络名，
                            temp_row.push_back(MTV[index][1]);    // 一STV
                            temp_row.push_back(MRV[index][1]);    // 二SRV
                            temp_matrix.push_back(temp_row);
                            temp_row.clear();
                        }
                        confounding.push_back(temp_matrix);
                        // 把处理过的SRV的flag置2
                        for(auto &index:equal_srv_index)
                        {
                            flag[index]=2;
                        }
                        break;
                }
                }
            }
        }

        ++ii;
    }
}

// =================================私有函数：对STV进行按位与========================================================
// 由于SRV可能很长（几十位甚至几百位），将QString转成longlong也可能装不下，所以自己写个函数一位一位与
QString Analyze_MTV::bit_and(QString &a,QString &b)
{
    QString c;
    int ii =0;
    for(const auto &i:a)
    {
        // 将字符串a和b的对应位先转换成int，再做按位与运算
        int int_bit =( i.digitValue() & (b[ii]).digitValue());

        if (int_bit==1)
        {
            c.push_back('1');
        }
        else
        {
            c.push_back('0');
        }
        ++ii;
    }
    return c;
}

// =================================私有函数：对STV进行按位或========================================================
QString Analyze_MTV::bit_or(QString &a,QString &b)
{
    QString c;
    int ii =0;
    for(const auto &i:a)
    {
        // 将字符串a和b的对应位先转换成int，再做按位与运算
        int int_bit =( i.digitValue() | (b[ii]).digitValue());

        if (int_bit==1)
        {
            c.push_back('1');
        }
        else
        {
            c.push_back('0');
        }
        ++ii;
    }
    return c;
}

// =================================私有函数：分析各种比率===========================================================
// (1)故障检测率
// (2)故障隔离率
void Analyze_MTV::analyze_rate()
{
//   // 征兆误判率=误判征兆数/单一短路故障总数
//   double net_num = MTV.size();
//   double single_short_fault_num = pow(2,net_num) - net_num -1;  //2的net_num次方太大了，计算这个根本不实际
//   double aliasing_num = aliasing.size();
//   aliasing_rate = aliasing_num/single_short_fault_num;
//   // 征兆混淆率=征兆混淆数/组合短路故障总数
//   double confounding_num = 0;

    // (1)故障检测率 = 正确检测出的故障数/发生的故障总数
    // 呆滞0的网络数
    int s_a_0_num = s_a_0_fault.size();
    // 呆滞1的网络数
    int s_a_1_num = s_a_1_fault.size();
    // 发生线或短路的网络个数
    int w_o_num = 0;
    for(const auto &i:w_o_fault)
    {
        w_o_num += i.size();
    }
    // 发生线与短路的网络个数
    int w_a_num = 0;
    for(const auto &i:w_a_fault)
    {
        w_a_num += i.size();
    }
    // 不知道是什么的短路的网络数
    int unknow_short_fault_num = 0;
    for(const auto &i:unknow_short_fault)
    {
        unknow_short_fault_num += i.size();
    }
    // 不知道类型的故障
    int unknow_fault_num =unknow_fault.size();
    // 征兆误判的网络个数
    int aliasing_num = 0;
    for(const auto &i:aliasing)
    {
        aliasing_num += i.size();
    }
    // 征兆混淆的网络数
    int confounding_num = 0;
    for(const auto &i:confounding)
    {
        confounding_num += i.size();
    }

    // 正确检测出的故障数
    double N_d = s_a_0_num +s_a_1_num + w_o_num +w_a_num + unknow_short_fault_num + unknow_fault_num +confounding_num;
    // 发生的故障总数（所有可能的故障）= 能正确检测出的故障数 + 误判的故障（可能有故障、可能没有故障）
    double N_t = N_d + aliasing_num;
    // 故障检测率，若分母为0，则故障检测率100%
    if(!N_t)
    {
       FDR = 1;
    }
    else
    {
       FDR = N_d / N_t;
    }

    // (2)故障隔离率 = 能被隔离出的故障数/能被正确检测出的故障数
    // 能被隔离出的故障数
    double N_L = s_a_0_num +s_a_1_num + w_o_num +w_a_num + unknow_short_fault_num + unknow_fault_num;
    // 故障隔离率，若分母为0，则故障隔离率100%
    if(!N_d)
    {
        FIR = 1;
    }
    else
    {
        FIR = N_L / N_d;
    }
}




Analyze_MTV::~Analyze_MTV()
{

}

// =================================将各种私有数据return出去，让外部能得到这些数据============================================
QVector<QVector<QString> > Analyze_MTV::get_MTV()
{
    return MTV;
}
QVector<QVector<QString> > Analyze_MTV::get_MRV()
{
    return MRV;
}

QVector<QVector<QString> > Analyze_MTV::get_s_a_0_fault()
{
    return s_a_0_fault;
}
QVector<QVector<QString> > Analyze_MTV::get_s_a_1_fault()
{
    return s_a_1_fault;
}
QVector<QVector<QVector<QString> > > Analyze_MTV::get_w_o_fault()
{
    return w_o_fault;
}
QVector<QVector<QVector<QString> > > Analyze_MTV::get_w_a_fault()
{
    return w_a_fault;
}
QVector<QVector<QString> > Analyze_MTV::get_unknow_fault()
{
    return unknow_fault;
}
QVector<QVector<QVector<QString> > > Analyze_MTV::get_unknow_short_fault()
{
    return unknow_short_fault;
}

QVector<QVector<QVector<QString> > > Analyze_MTV::get_aliasing()
{
    return aliasing;
}
QVector<QVector<QVector<QString> > > Analyze_MTV::get_confounding()
{
    return confounding;
}
double Analyze_MTV::get_FDR()
{
    return FDR;
}
double Analyze_MTV::get_FIR()
{
    return FIR;
}
//double Analyze_MTV::get_aliasing_rate()
//{
//    return aliasing_rate;
//}
//double Analyze_MTV::get_confounding_rate()
//{
//    return confounding_rate;
//}


