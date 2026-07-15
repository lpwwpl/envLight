#ifndef EPWDATA_H
#define EPWDATA_H

#include <QString>
#include <QVector>

struct EpwLocation {
    QString city;
    double latitude = 0.0;
    double longitude = 0.0;
    double timeZone = 0.0;
    double elevation = 0.0;
};

struct EpwRecord {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;          // 1-24 (结束时刻)
    int minute = 0;        // 0-60
    double dhi = 0.0;      // 水平散射辐照度 (W/m²)
    double dni = 0.0;      // 直射辐照度 (W/m²)
    double dryBulb = 0.0;  // 干球温度 (°C)
};

struct EpwDocument {
    EpwLocation location;
    QVector<EpwRecord> records;
    int recordsPerHour = 1;  // 每小时记录条数 (通常为1，子小时可能>1)
};

#endif // EPWDATA_H