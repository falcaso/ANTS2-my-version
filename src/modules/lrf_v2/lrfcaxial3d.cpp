#include "lrfcaxial3d.h"
#include "jsonparser.h"

#include <math.h>

LRFcAxial3d::LRFcAxial3d(double r, int nint, double zmin, double zmax,
                         int nintz, double k, double r0, double lam, double x0, double y0) :
    LRFaxial3d(r, nint, zmin, zmax, nintz, x0, y0)
{
    this->r0 = r0;
    a = (k+1)/(k-1);
    lam2 = lam*lam;
    b = sqrt(r0*r0+lam2)+a*r0;

    comp_lam = lam;
    comp_k = k;
}

LRFcAxial3d::LRFcAxial3d(QJsonObject &json) : LRFaxial3d(json)
{
    JsonParser parser(json);
    parser.ParseObject("a", a);
    parser.ParseObject("b", b);
    parser.ParseObject("r0", r0);
    parser.ParseObject("lam2", lam2);
    if(!parser.ParseObject("comp_lam", comp_lam))
        comp_lam = sqrt(lam2);
    if(!parser.ParseObject("comp_k", comp_k))
        comp_k = (a+1)/(a-1);
}

QJsonObject LRFcAxial3d::reportSettings() const
{
   QJsonObject json(LRFaxial3d::reportSettings());
   json["r0"] = r0;
   json["comp_lam"] = comp_lam;
   json["comp_k"] = comp_k;

   return json;
}

double LRFcAxial3d::Rho(double r) const
{
    double dr = r - r0;
    return std::max(0., b + dr*a - sqrt(dr*dr + lam2));
}

double LRFcAxial3d::Rho(double x, double y) const
{
    double dr = R(x, y) - r0;
    return std::max(0., b + dr*a - sqrt(dr*dr + lam2));
}

double LRFcAxial3d::RhoDrvX(double x, double y) const
{
    double dr = R(x, y) - r0;
    double dRhodR = dr/sqrt(dr*dr + lam2) + a;
    return dRhodR*(x-x0)/R(x, y);
}

double LRFcAxial3d::RhoDrvY(double x, double y) const
{
    double dr = R(x, y) - r0;
    double dRhodR = dr/sqrt(dr*dr + lam2) + a;
    return dRhodR*(y-y0)/R(x, y);
}

void LRFcAxial3d::writeJSON(QJsonObject &json) const
{
    LRFaxial3d::writeJSON(json);
    json["type"] = QString(type());
    json["a"] = a;
    json["b"] = b;
    json["r0"] = r0;
    json["lam2"] = lam2;
    json["comp_lam"] = comp_lam;
    json["comp_k"] = comp_k;
}

