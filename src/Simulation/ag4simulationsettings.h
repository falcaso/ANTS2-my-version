#ifndef AG4SIMULATIONSETTINGS_H
#define AG4SIMULATIONSETTINGS_H

#include <QString>
#include <QStringList>
#include <QMap>

class QJsonObject;

class AG4SimulationSettings
{
public:
    AG4SimulationSettings();

    bool bTrackParticles = false;

    QString               PhysicsList = "QGSP_BERT_HP";
    QStringList           SensitiveVolumes; //later will be filled automatically
    QStringList           Commands;
    QMap<QString, double> StepLimits;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);

    const QString getPrimariesFileName(int iThreadNum) const;
    const QString getDepositionFileName(int iThreadNum) const;
    const QString getReceitFileName(int iThreadNum) const;
    const QString getConfigFileName(int iThreadNum) const;
    const QString getTracksFileName(int iThreadNum) const;
    const QString getGdmlFileName() const;

private:
    const QString getPath() const;

};

#endif // AG4SIMULATIONSETTINGS_H
