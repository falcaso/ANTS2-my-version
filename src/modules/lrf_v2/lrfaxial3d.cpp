#include "lrfaxial3d.h"
#include "spline.h"
#include "jsonparser.h"
#include "bsfit123.h"

#include <math.h>

#include <QJsonObject>

LRFaxial3d::LRFaxial3d(double r, int nint, double zmin, double zmax,
            int nintz, double x0, double y0) : LRFaxial(r, nint, x0, y0)
{
    this->zmin = zmin;
    this->zmax = zmax;
    this->nintz = nintz;
    bs2r = new Bspline2d(0., r, nint, zmin, zmax, nintz);
    bs2e = new Bspline2d(0., r, nint, zmin, zmax, nintz);
    z_slope = 0;
}

LRFaxial3d::LRFaxial3d(QJsonObject &json) : LRFaxial(json), bs2r(NULL), bs2e(NULL)
{
    JsonParser parser(json);
    QJsonObject jsobj, splineobj;

//    x0 = y0 = 0.; // compatibility: default axis is at (0,0)
//    parser.ParseObject("x0", x0);
//    parser.ParseObject("y0", y0);
//    parser.ParseObject("rmax", rmax);
    parser.ParseObject("zmin", zmin);
    parser.ParseObject("zmax", zmax);
//    recalcLimits();

// read response
    if ( parser.ParseObject("response", jsobj) ) {
        JsonParser tmp(jsobj);
        if (tmp.ParseObject("tpspline3", splineobj))
            bs2r = read_tpspline3_json(splineobj);
    }

// try to read error
    if ( parser.ParseObject("error", jsobj) ) {
        JsonParser tmp(jsobj);
        if (tmp.ParseObject("tpspline3", splineobj))
            bs2e = read_tpspline3_json(splineobj);
    }

// final touches
    valid = (!parser.GetErrType() && bs2r) ? true : false;
    if (bs2r) {
        nint = bs2r->GetNintX();
        nintz = bs2r->GetNintY();
    }
}

LRFaxial3d::~LRFaxial3d()
{
    if (bs2r)
        delete bs2r;
    if (bs2e)
        delete bs2e;
}

bool LRFaxial3d::errorDefined() const
{
    return bs2e->IsReady();
}

bool LRFaxial3d::isReady() const
{
    return (bs2r != 0) && bs2r->IsReady();
}

bool LRFaxial3d::inDomain(double x, double y, double z) const
{
    return x*x + y*y < rmax2 && z>zmin && z<zmax;
}

double LRFaxial3d::eval(double x, double y, double z) const
{
    return isReady() ? bs2r->Eval(Rho(x, y), z) : 0.;
}

double LRFaxial3d::evalErr(double x, double y, double z) const
{
    return errorDefined() ? bs2e->Eval(Rho(x, y), z) : 0.;
}

double LRFaxial3d::evalDrvX(double x, double y, double z) const
{
    return isReady() ? bs2r->EvalDrvX(Rho(x, y), z)*RhoDrvX(x, y) : 0.;
}

double LRFaxial3d::evalDrvY(double x, double y, double z) const
{
    return isReady() ? bs2r->EvalDrvX(Rho(x, y), z)*RhoDrvY(x, y) : 0.;
}

double LRFaxial3d::eval(double x, double y, double z, double *err) const
{
    double rho = Rho(x, y);
    *err = errorDefined() ? bs2e->Eval(rho, z) : 0.;
    return isReady() ? bs2r->Eval(rho, z) : 0.;
}

double LRFaxial3d::fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid)
{
    std::vector <double> vr;
    std::vector <double> vz;
    std::vector <double> va;

    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i], z[i]))
            continue;
        vr.push_back(Rho(x[i], y[i]));
        vz.push_back(z[i]);
        va.push_back(data[i]);
    }

    bs2r = new Bspline2d(0., Rho(rmax), nint, zmin, zmax, nintz);

    BSfit2D F(bs2r);
    if (flattop) F.SetConstraintDdxAt0();
    if (non_negative) F.SetConstraintNonNegative();
    if (non_increasing) F.SetConstraintNonIncreasingX();
    if (z_slope) F.SetConstraintSlopeY(z_slope);

    if (!grid) {
        F.Fit(va.size(), &vr[0], &vz[0], &va[0]);
        valid = bs2r->IsReady();
        return F.GetResidual();
    } else {
        F.AddData(va.size(), &vr[0], &vz[0], &va[0]);
        F.Fit();
        valid = bs2r->IsReady();
        return F.GetResidual();
    }
}

double LRFaxial3d::fitError(int npts, const double *x, const double *y, const double *z, const double *data, bool grid)
{
    std::vector <double> vr;
    std::vector <double> vz;
    std::vector <double> va;

    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i], z[i]))
            continue;
        double rc = Rho(x[i], y[i]);
        vr.push_back(rc);
        vz.push_back(z[i]);
        double err = data[i] - bs2r->Eval(rc, z[i]);
//        va.push_back(fabs(err));
        va.push_back(err*err);
    }

    bs2e = new Bspline2d(0., Rho(rmax), nint, zmin, zmax, nintz);

    BSfit2D F(bs2e);

    if (!grid) {
        F.Fit(va.size(), &vr[0], &vz[0], &va[0]);
         return F.GetResidual();
    } else {
        F.AddData(va.size(), &vr[0], &vz[0], &va[0]);
        F.Fit();
        return F.GetResidual();
    }
}


void LRFaxial3d::setSpline(Bspline2d *bs)
{
    if (bs2r)
        delete bs2r;

    bs2r = bs;
    rmax = bs2r->GetXmax();
    nint = bs2r->GetNintX();
    zmin = bs2r->GetYmin();
    zmax = bs2r->GetYmax();
    nintz = bs2r->GetNintY();
    recalcLimits();
}

const Bspline2d *LRFaxial3d::getSpline() const
{
    return bs2r;
}

void LRFaxial3d::writeJSON(QJsonObject &json) const
{
    json["type"] = QString(type());
    json["rmax"] = rmax;
    json["x0"] = x0;
    json["y0"] = y0;
    json["zmin"] = zmin;
    json["zmax"] = zmax;
    if (bs2r) {
        QJsonObject response, spline;
        write_tpspline3_json(bs2r, spline);
        response["tpspline3"] = spline;
        json["response"] = response;
    }
    if (bs2e) {
        QJsonObject error, spline;
        write_tpspline3_json(bs2e, spline);
        error["tpspline3"] = spline;
        json["error"] = error;
      }
}

QJsonObject LRFaxial3d::reportSettings() const
{
  QJsonObject json;
  json["type"] = QString(type());
  json["n1"] = nint;
  json["n2"] = nintz;

  return json;
}


