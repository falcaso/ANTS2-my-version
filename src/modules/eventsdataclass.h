#ifndef EVENTSDATACLASS_H
#define EVENTSDATACLASS_H

#ifdef SIM
#include "asimulationstatistics.h"
#include "ahistoryrecords.h"
#endif

#include "generalsimsettings.h"
#include "reconstructionsettings.h"
#include "manifesthandling.h"

#include <QVector>
#include <QObject>
#include <TString.h>

class TTree;
class pms;
struct AScanRecord;
struct AReconRecord;
class TRandom2;

class EventsDataClass : public QObject
{
  Q_OBJECT
public:
    EventsDataClass(const TString nameID = ""); //nameaddon to make unique hist names in multithread
    ~EventsDataClass();

    // True/calibration positions
    QVector<AScanRecord*> Scan;
    int ScanNumberOfRuns; //number of runs performed at each position (node) - see simulation module
    bool isScanEmpty() {return Scan.isEmpty();}
    void clearScan();
    void purge1e10events();  //added after introduction of multithread to remove nodes outside the defined volume - they are marked as true x and y = 1e10

    // PM signal data
    QVector< QVector <float> > Events; //[event][pm]  - remember, if events energy is loaded, one MORE CHANNEL IS ADDED: last channel is numPMs+1
    QVector< QVector < QVector <float> > > TimedEvents; //event timebin pm
    bool isEmpty() {return Events.isEmpty();}
    bool isTimed() {return !TimedEvents.isEmpty();}
    int getTimeBins();
    int getNumPMs();
    const QVector<float> *getEvent(int iev);
    const QVector<QVector<float> > *getTimedEvent(int iev);

#ifdef SIM
    // Logs
    QVector<EventHistoryStructure*> EventHistory;
    QVector<GeneratedPhotonsHistoryStructure> GeneratedPhotonsHistory;

    //Detection statistics
    ASimulationStatistics* SimStat;
    bool isStatEmpty() {return SimStat->isEmpty();}
    void setDetStatNumBins(int numBins);
    int  getStatDataNumBins() {return DetStatNumBins;}
#endif

    //Reconstruction data
    QVector< QVector<AReconRecord*> > ReconstructionData;  // [sensor_group] : [event]
    //QVector<bool> fReconstructionDataReady; // true if reconstruction was already performed for the group
    bool fReconstructionDataReady; // true if reconstruction was already performed for the group
    bool isReconstructionDataEmpty(int igroup = 0);  // container is empty
    bool isReconstructionReady(int igroup = 0);  // reconstruction was not yet performed
    void clearReconstruction();
    void clearReconstruction(int igroup);
    void createDefaultReconstructionData(int igroup = 0); //recreates new container - clears data completely!
    void resetReconstructionData(int numGgroups);         //reinitialize numPoints to 1 for each event. Keeps eventId intact!
    bool BlurReconstructionData(int type, double sigma, TRandom2 *RandGen, int igroup = -1); // 0 - uniform, 1 - gauss; igroup<0 -> apply to all groups
    bool BlurReconstructionDataZ(int type, double sigma, TRandom2 *RandGen, int igroup = -1); // 0 - uniform, 1 - gauss; igroup<0 -> apply to all groups
    void PurgeFilteredEvents(int igroup = 0);
    void Purge(int OnePer, int igroup = 0);    
    int countGoodEvents(int igroup = 0);
    void copyTrueToReconstructed(int igroup = 0);
    void prepareStatisticsForEvents(const bool isAllLRFsDefined, int &GoodEvents, double &AvChi2, double &AvDeviation, int igroup = 0);

    //load data can have manifest file with holes/slits
    QVector<ManifestItemBaseClass*> Manifest;
    void clearManifest();

    // simulation setting for events / scan
    GeneralSimSettings LastSimSet;   
    bool fSimulatedData; // true if events data were simulated (false if loaded data)

    // reconstruction settings used during last reconstruction
    QVector<ReconstructionSettings> RecSettings;

    //Trees
    TTree *ReconstructionTree; //tree with reconstruction data
    void clearReconstructionTree();
    bool createReconstructionTree(pms* PMs,
                                  bool fIncludePMsignals=true,
                                  bool fIncludeRho=true,
                                  bool fIncludeTrue=true,
                                  int igroup = 0);
    TTree* ResolutionTree; //tree with resolution data
    void clearResolutionTree();
    bool createResolutionTree(int igroup = 0);

    //Data Save
    bool saveReconstructionAsTree(QString fileName,
                                  pms *PMs,
                                  bool fIncludePMsignals=true,
                                  bool fIncludeRho = true,
                                  bool fIncludeTrue = true,                                  
                                  int igroup = 0);
    bool saveReconstructionAsText(QString fileName, int igroup=0);
    bool saveSimulationAsTree(QString fileName);
    bool saveSimulationAsText(QString fileName);

    //Data Load - ascii
    bool fLoadedEventsHaveEnergyInfo;
    int loadEventsFromTxtFile(QString fileName, QJsonObject &jsonPreprocessJson, pms *PMs); //returns -1 if failed, otherwise number of events added

    //data load - Tree
    int loadSimulatedEventsFromTree(QString fileName, pms *PMs, int maxEvents = -1); //returns -1 if failed, otherwise number of events added
    bool overlayAsciiFile(QString fileName, bool fAddMulti, pms *PMs); //true = success, if not, see ErrorString

    //Misc
    QString ErrorString;

    void squeeze();          

public slots:
    void clear();
    void onRequestStopLoad();

private:
    int DetStatNumBins;
    int fStopLoadRequested;

signals:
    void loaded(int events, int progress);
    void requestClearKNNfilter();
    void requestGuiUpdateForClearData();
    void requestEventsGuiUpdate();
};

#endif // EVENTSDATACLASS_H
