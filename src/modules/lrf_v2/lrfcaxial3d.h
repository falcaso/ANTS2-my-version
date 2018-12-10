#ifndef LRFCAXIAL3D_H
#define LRFCAXIAL3D_H

#include "lrfaxial3d.h"

class LRFcAxial3d : public LRFaxial3d
{
public:
    LRFcAxial3d(double r, int nint, double zmin, double zmax,
                int nintz, double k, double r0, double lam, double x0 = 0., double y0 = 0.);
    virtual void writeJSON(QJsonObject &json) const;
    LRFcAxial3d(QJsonObject &json);
    virtual const char *type() const { return "ComprAxial3D"; }
    virtual QJsonObject reportSettings() const;

    virtual double Rho(double r) const;
    virtual double Rho(double x, double y) const;
    virtual double RhoDrvX(double x, double y) const;
    virtual double RhoDrvY(double x, double y) const;

protected:
    double a;
    double b;
    double r0;
    double lam2;

    double comp_k;
    double comp_lam;
};

#endif // LRFCAXIAL3D_H
