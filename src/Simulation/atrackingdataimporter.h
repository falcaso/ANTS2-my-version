#ifndef ATRACKINGDATAIMPORTER_H
#define ATRACKINGDATAIMPORTER_H

#include <QString>
#include <QVector>
#include <QMap>
#include <vector>

class AEventTrackingRecord;
class AParticleTrackingRecord;
class TrackHolderClass;
class ATrackBuildOptions;

class ATrackingDataImporter
{
public:
    ATrackingDataImporter(const ATrackBuildOptions & TrackBuildOptions, std::vector<AEventTrackingRecord*> * History, QVector<TrackHolderClass*> * Tracks);

    const QString processFile(const QString & FileName, int StartEvent);

private:
    const ATrackBuildOptions & TrackBuildOptions;
    std::vector<AEventTrackingRecord*> * History = nullptr; // if 0 - do not collect history
    QVector<TrackHolderClass*> * Tracks = nullptr;          // if 0 - do not extract tracks

    QString currentLine;
    AEventTrackingRecord * CurrentEventRecord = nullptr;      // history of the current event
    AParticleTrackingRecord * CurrentParticleRecord = nullptr;  // current particle - can be primary or secondary
    QMap<int, AParticleTrackingRecord *> PromisedSecondaries;   // <index in file, secondary AEventTrackingRecord *>
    TrackHolderClass * CurrentTrack = nullptr;

    enum Status {ExpectingEvent, ExpectingTrack, ExpectingStep, TrackOngoing};

    Status CurrentStatus = ExpectingEvent;
    int ExpectedEvent;

    QString Error;

    void processNewEvent();
    void processNewTrack();
    void processNewStep();

    bool isPromisesFailed();

};

class ATrackingImportStateMachine
{
public:
    ATrackingImportStateMachine(QString & Error) : Error(Error) {}
    virtual ~ATrackingImportStateMachine(){}

    virtual void processNewEvent() = 0;
    virtual void processNewParticle() = 0;
    virtual void processNewStep() = 0;

protected:
    QString & Error;
};

#endif // ATRACKINGDATAIMPORTER_H
