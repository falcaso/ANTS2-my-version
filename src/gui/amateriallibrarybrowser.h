#ifndef AMATERIALLIBRARYBROWSER_H
#define AMATERIALLIBRARYBROWSER_H

#include <QDialog>
#include <QSet>
#include <QString>
#include <QDir>

namespace Ui {
class AMaterialLibraryBrowser;
}

struct AMaterialLibraryRecord
{
    int           Index;
    QString       FileName;
    QString       MaterialName;
    QSet<QString> Tags;

    AMaterialLibraryRecord(int Index, const QString &FileName, const QString &MaterialName);
    AMaterialLibraryRecord(){}
};

struct ATagRecord
{
    QString Tag;
    bool    bChecked = false;

    ATagRecord(const QString & Tag);
    ATagRecord(){}
};

class AMaterialLibraryBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit AMaterialLibraryBrowser(QWidget *parent = 0);
    ~AMaterialLibraryBrowser();

private slots:
    void on_pbDummy_clicked();

    void on_pbLoad_clicked();

    void on_pbClose_clicked();

    void on_pbClearTags_clicked();

private:
    Ui::AMaterialLibraryBrowser *ui;
    QDir Dir;
    QVector<AMaterialLibraryRecord> MaterialRecords;
    QSet<QString> Tags;
    QVector<ATagRecord> TagRecords;

private:
    void updateGui();
    void readFiles();
};

#endif // AMATERIALLIBRARYBROWSER_H
