#include "anoderecord.h"

#include <QDebug>

ANodeRecord::ANodeRecord(double x, double y, double z, double time, int numPhot, ANodeRecord * rec) :
    Time(time), NumPhot(numPhot), LinkedNode(rec)
{
    R[0] = x;
    R[1] = y;
    R[2] = z;
}

ANodeRecord::ANodeRecord(double * r, double time, int numPhot, ANodeRecord *rec) :
    Time(time), NumPhot(numPhot), LinkedNode(rec)
{
    for (int i=0; i<3; i++) R[i] = r[i];
}

ANodeRecord::~ANodeRecord()
{
    //delete LinkedNode;  // recursive call of the destructor kills the stack if too many

    const int numNodes = getNumberOfLinkedNodes();
    QVector<ANodeRecord*> allLinkedNodes;
    allLinkedNodes.reserve(numNodes);

    ANodeRecord * node = this;
    while (node->LinkedNode)
    {
        ANodeRecord * ln = node->LinkedNode;
        allLinkedNodes << ln;
        node->LinkedNode = nullptr;
        node = ln;
    }

    for (ANodeRecord * node : allLinkedNodes)
        delete node;
}

ANodeRecord *ANodeRecord::createS(double x, double y, double z, double time, int numPhot, ANodeRecord * rec)
{
    return new ANodeRecord(x, y, z, time, numPhot, rec);
}

ANodeRecord *ANodeRecord::createV(double *r, double time, int numPhot, ANodeRecord *rec)
{
    return new ANodeRecord(r, time, numPhot, rec);
}

// #include <iostream> // test only

// optimized FN @ 2021.05.20
void ANodeRecord::addLinkedNode(ANodeRecord * node)
{
// original
//    ANodeRecord * n = this;
//    while (n->LinkedNode) n = n->LinkedNode;
//    n->LinkedNode = node;

// may be problematic if not starting from the front.
// ANodeRecord * n = this;
// if (FBack) n=FBack;
// else while (n->LinkedNode) n = n->LinkedNode;

 // more general, if we don't start from the front
 ANodeRecord * n = FBack?FBack:this;
 while (n->LinkedNode) n = n->LinkedNode;

// // check
// ANodeRecord * m = this;
// while (m->LinkedNode) m = m->LinkedNode;
// if (m!=n) std::cout << m << " " << n << " " << (m==n) << std::endl << std::flush;

 FBack = n->LinkedNode = node;
}

int ANodeRecord::getNumberOfLinkedNodes() const
{
    int counter = 0;
    const ANodeRecord * node = this;
    while (node->LinkedNode)
    {
        counter++;
        node = node->LinkedNode;
    }
    return counter;
}
