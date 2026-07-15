#include "EpwReader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

bool EpwReader::read(const QString& filePath, EpwDocument& doc)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open EPW file:" << filePath;
        return false;
    }

    QTextStream in(&file);
    doc.records.clear();

    // 读取头 8 行
    QStringList headers;
    for (int i = 0; i < 8; ++i) {
        if (in.atEnd()) return false;
        headers << in.readLine();
    }

    // 解析第 1 行：地点信息
    const QString& locLine = headers[0];
    QStringList parts = locLine.split(',');
    if (parts.size() >= 7) {
        doc.location.city = parts[0];
        doc.location.latitude = parts[1].toDouble();
        doc.location.longitude = parts[2].toDouble();
        doc.location.timeZone = parts[3].toDouble();
        doc.location.elevation = parts[4].toDouble();
    }

    // 判断记录频率：检查第 9 行是否有子小时数据（通过 minute 列判断）
    // 这里简化，默认每小时一条记录
    doc.recordsPerHour = 1;

    // 读取数据体
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        QStringList fields = line.split(',');

        if (fields.size() < 35) continue; // EPW 至少 35 列

        EpwRecord rec;
        rec.year = fields[0].toInt();
        rec.month = fields[1].toInt();
        rec.day = fields[2].toInt();
        rec.hour = fields[3].toInt();
        rec.minute = fields[4].toInt();

        // 散射辐射 (列 14, 0-based index=14)
        rec.dhi = fields[14].toDouble();
        // 直射辐射 (列 15)
        rec.dni = fields[15].toDouble();
        // 干球温度 (列 6)
        rec.dryBulb = fields[6].toDouble();

        doc.records.push_back(rec);
    }

    // 如果记录条数是 8760 或 8760*子小时，可自动检测
    // 这里不自动调整 recordsPerHour，默认 1
    return true;
}