#ifndef APARTICLESOURCESIMULATOR_H
#define APARTICLESOURCESIMULATOR_H

#include "asimulator.h"
#include "aparticlesimsettings.h"
#include <vector>

#include <QVector>

#include <QObject>
#include <QProcess>

class AEnergyDepositionCell;
class AParticleRecord;
class AParticleTracker;// PrimaryParticleTracker;
class S1_Generator;
class S2_Generator;
class AParticleGun;
class QProcess;
class AEventTrackingRecord;
class AExternalProcessHandler;
class QTextStream;
class QFile;

class AParticleSourceSimulator : public ASimulator
{
public:
    explicit AParticleSourceSimulator(ASimulationManager & simMan, const AParticleSimSettings & partSimSet, int ID);
    ~AParticleSourceSimulator();

    const QVector<AEnergyDepositionCell*> &getEnergyVector() const { return EnergyVector; }  // !*! to change
    void ClearEnergyVectorButKeepObjects() {EnergyVector.resize(0);} //to avoid clear of objects stored in the vector  // !*! to change

    virtual int getEventCount() const override { return eventEnd - eventBegin; }
    virtual int getEventsDone() const override;
    virtual int getTotalEventCount() const override { return totalEventCount; }
    virtual bool setup(QJsonObject & json) override;
    virtual bool finalizeConfig() override;
    virtual void updateGeoManager() override;

    virtual void simulate() override;

    virtual void appendToDataHub(EventsDataClass * dataHub) override;
    virtual void mergeData() override;

    //void setOnlySavePrimaries() {bOnlySavePrimariesToFile = true;} // for G4ants mode // obsolete? *!*
    const AParticleGun * getParticleGun() const {return ParticleGun;}  // !*! to remove

    virtual void hardAbort() override;

protected:
    virtual void updateMaxTracks(int maxPhotonTracks, int maxParticleTracks) override;


private:
    void EnergyVectorToScan();
    void clearParticleStack();
    void clearEnergyVector();
    void clearGeneratedParticles();

    int  chooseNumberOfParticlesThisEvent() const;
    bool choosePrimariesForThisEvent(int numPrimaries, int iEvent);
    bool generateAndTrackPhotons();
    bool geant4TrackAndProcess();
    bool runGeant4Handler();

    bool processG4DepositionData();
    bool readG4DepoEventFromTextFile();
    bool readG4DepoEventFromBinFile(bool expectNewEvent = false);
    void releaseInputResources();
    bool prepareWorkerG4File();

private:
    const AParticleSimSettings & partSimSet;

    //local objects
    AParticleTracker * ParticleTracker = nullptr;
    S1_Generator     * S1generator     = nullptr;
    S2_Generator     * S2generator     = nullptr;
    AParticleGun     * ParticleGun     = nullptr;

    QVector<AEnergyDepositionCell*> EnergyVector;
    QVector<AParticleRecord*>       ParticleStack;

    //resources for ascii input
    QFile         * inTextFile    = nullptr;
    QTextStream   * inTextStream  = nullptr;
    QString         G4DepoLine;
    //resources for binary input
    std::ifstream * inStream      = nullptr;
    int             G4NextEventId = -1;

    //local use - container which particle generator fills for each event; the particles are deleted by the tracker
    QVector<AParticleRecord*> GeneratedParticles;

    int totalEventCount = 0;
    double timeFrom, timeRange;   // !*! to remove
    double updateFactor;

    //Control
    //bool fDoS1;
    //bool fDoS2;
    //bool fAllowMultiple; //multiple particles per event?
    //int AverageNumParticlesPerEvent;
    //int TypeParticlesPerEvent;  //0 - constant, 1 - Poisson
    //bool fIgnoreNoHitsEvents;
    //bool fIgnoreNoDepoEvents;
    //bool bClusterMerge = true;
    //double ClusterMergeRadius2 = 1.0; //scan cluster merge radius [mm] in square - used by EnergyVectorToScan()
    //double ClusterMergeTimeDif = 1.0;

    //Geant4 interface
    AExternalProcessHandler * G4handler = nullptr;
    bool bOnlySavePrimariesToFile = false;     // !*! obsolete?
    bool bG4isRunning = false;
    QSet<QString> SeenNonRegisteredParticles;
    double DepoByNotRegistered = 0;
    double DepoByRegistered = 0;
    std::vector<AEventTrackingRecord *> TrackingHistory;

};

#endif // APARTICLESOURCESIMULATOR_H
