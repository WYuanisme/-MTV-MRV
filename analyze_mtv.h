//--------------------------------------------------------------------
// Code by WangYuan; Date:2019/11/12
// 输入：MTV、MRV文本文件
// 输出：短路、呆滞故障，误判、混淆网络，误判率、混淆率
//--------------------------------------------------------------------


#ifndef ANALYZE_MTV_H
#define ANALYZE_MTV_H

#include <QString>
#include <QVector>


class Analyze_MTV
{
private:
    QVector<QVector<QString> > MTV;      // 把网络名和STV，存进容器MTV。
    QVector<QVector<QString> > MRV;      // 把网络名和SRV，存进容器MRV
    QVector<int> flag;                        // 第i个元素对应MRV中的第i行SRV,SRV不等于STV则该元素值为1，否则为0。若该行已经处理过，元素值为2

    QVector<QVector<QString> > s_a_0_fault;        // 保存呆滞0故障网络，每行，第0个元素为网络名，1为STV，2为SRV
    QVector<QVector<QString> > s_a_1_fault;        // 保存呆滞1故障网络，每行，第0个元素为网络名，1为STV，2为SRV    

    QVector<QVector<QVector<QString> > > w_o_fault;       // 保存线或短路故障，一个矩阵存某几个短在一起的网络，每行一个网络，第0个元素为网络名，一STV，二SRV。
    QVector<QVector<QVector<QString> > > w_a_fault;       // 保存线与短路故障，存放方法与线或一样

    QVector<QVector<QString> > unknow_fault;    // 不知道类型的故障
    QVector<QVector<QVector<QString> > > unknow_short_fault; // 既不是线与也不是线或的短路
    //   QVector<QVector<QString> > s_d_k_fault;     // 强驱动逻辑故障？？？不知道怎么和征兆误判区分

    QVector<QVector<QVector<QString> > > aliasing;        // 征兆误判的网络，一个矩阵存误判的网络和其发生误判的其他网络。每行，第0个元素为网络名，一STV，二SRV
    QVector<QVector<QVector<QString> > > confounding;     // 征兆混淆的网络，一个矩阵存某几个混淆的网络。每行，第0个元素为网络名，一STV，二SRV

    //    double aliasing_rate;                          // 征兆误判率
    //    double confounding_rate;                       // 征兆混淆率

    double FDR;     // 故障检测率 fault detect rate
    double FIR;     // 故障隔离率 fault isolation rate




    // 输入文本文件名，把文件内容存进容器，若修改后的矩阵为空，返回0，表示操作失败，否则返回1
    bool get_file_content(const QString &file_path,QVector<QVector<QString> > &matrix);
    bool get_flag_srv_diff_to_stv();     // 找出所有与STV不同的SRV，修改flag。
    void analyze_fault();                // 遍历flag，若元素==1，找到对应MRV行，分析故障。
    void analyze_rate();
    QString bit_and(QString &a,QString &b);      // 对STV进行按位与，由于SRV可能很长（几十位甚至更长），将QString转成longlong也可能装不下，所以自己写个函数一位一位与
    QString bit_or(QString &a,QString &b);       // 对STV进行按位或

public:
    Analyze_MTV(const QString &MTV_file_path,const QString &MRV_file_path);
    Analyze_MTV()=default;
    virtual ~Analyze_MTV();

    QVector<QVector<QString> > get_MTV();
    QVector<QVector<QString> > get_MRV();
    QVector<QVector<QString> > get_s_a_0_fault();
    QVector<QVector<QString> > get_s_a_1_fault();
    QVector<QVector<QVector<QString> > > get_w_o_fault();
    QVector<QVector<QVector<QString> > > get_w_a_fault();  
    QVector<QVector<QString> > get_unknow_fault();
    QVector<QVector<QVector<QString> > > get_unknow_short_fault();
    QVector<QVector<QVector<QString> > > get_aliasing();
    QVector<QVector<QVector<QString> > > get_confounding();
    double get_FDR();
    double get_FIR();
//    double get_aliasing_rate();
//    double get_confounding_rate();

};

#endif // ANALYZE_MTV_H
