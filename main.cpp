//--------------------------------------------------------------------
// Code by WangYuan; Date:2019/11/12
// 类：Analyze_MTV
// 输入：MTV、MRV文本文件
// 输出：短路、呆滞、强驱动、未知故障，误判、混淆网络，检测率、隔离率
//--------------------------------------------------------------------
#include <QCoreApplication>
#include <QDebug>
#include "analyze_mtv.h"

// 看输出调试
int main(int argc, char *argv[])
{
//    QCoreApplication a(argc, argv);
//    return a.exec();

    QString file_path1="C:/Users/10057/Desktop/123/MTV1.txt";
    QString file_path2="C:/Users/10057/Desktop/123/MRV1.txt";

    Analyze_MTV *p = new Analyze_MTV(file_path1,file_path2);
    auto a = p->get_w_a_fault();
    auto b = p->get_w_o_fault();

    for(auto &i:a)
    {
        qDebug()<<"\nw_a短路网络：";
        for(auto &j:i)
        {
              qDebug()<<j;
        }
    }
    for(auto &i:b)
    {
         qDebug()<<"\nw_o短路网络：";
        for(auto &j:i)
        {
              qDebug()<<j;
        }
    }

    auto c = p->get_FDR();
    auto d = p->get_FIR();
    qDebug()<< "\nFDR= "<<c<<"; FIR= "<< d;

    auto e = p->get_s_a_0_fault();
    auto f = p->get_s_a_1_fault();
    for(auto & i : e)
    {
        qDebug()<<"\n呆滞0网络："<<i;

    }
    for(auto & i : f)
    {
        qDebug()<<"\n呆滞1网络："<<i;

    }

    auto g = p->get_unknow_fault();
    auto h = p->get_unknow_short_fault();
    for(auto & i : g)
    {
        qDebug()<<"\n不知道什么故障网络："<<i;

    }
    for(auto &i:h)
    {
         qDebug()<<"\n不是线与线或的短路网络：";
        for(auto &j:i)
        {
              qDebug()<<j;
        }
    }

    auto k = p->get_aliasing();
    auto l = p->get_confounding();
    for(auto &i:k)
    {
         qDebug()<<"\n征兆误判网络：";
        for(auto &j:i)
        {
              qDebug()<<j;
        }
    }
    for(auto &i:l)
    {
         qDebug()<<"\n征兆混淆网络：";
        for(auto &j:i)
        {
              qDebug()<<j;
        }
    }




    delete p;
    p = nullptr;
}
