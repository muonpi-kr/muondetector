#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSsl>
#include <cmath>
#include <map.h>
#include <muondetector_structs.h>

#include <ui_map.h>

Map::Map(QWidget* parent)
    : QWidget(parent)
    , mapUi(new Ui::Map)
{
    QVariantMap parameters;
    mapUi->setupUi(this);

    for ( const auto& item: PositionModeConfig::name ) {
        mapUi->modeComboBox->addItem(QString::fromLocal8Bit(item));
    }

    mapUi->mapWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    QQmlEngine* engine = new QQmlEngine(this);
    QQmlComponent* component = new QQmlComponent(engine, "qrc:/qml/CustomMap.qml");
    mapComponent = component->create();
    mapUi->mapWidget->setContent(component->url(), component, mapComponent);
    mapUi->mapWidget->show();
}

Map::~Map()
{
    delete mapComponent;
    delete mapUi;
}

void Map::onGeodeticPosReceived(const GnssPosStruct& pos)
{
    if (mapComponent == nullptr) {
        return;
    }
    double pos_error { std::sqrt(pos.hAcc * pos.hAcc + pos.vAcc * pos.vAcc) };
    QMetaObject::invokeMethod(mapComponent, "setCoordinates",
        Q_ARG(QVariant, ((double)pos.lon) * 1e-7),
        Q_ARG(QVariant, ((double)pos.lat) * 1e-7),
        Q_ARG(QVariant, ((double)pos.hMSL) * 1e-3),
        Q_ARG(QVariant, ((double)pos.hAcc) * 1e-3),
        Q_ARG(QVariant, ((double)pos.vAcc) * 1e-3),
        Q_ARG(QVariant, pos_error * 1e-3));
}

void Map::onPosConfigReceived(const PositionModeConfig &pos)
{
    mapUi->positionModelGroupBox->setEnabled(true);
    mapUi->staticPositionGroupBox->setEnabled(true);
    mapUi->setConfigPushButton->setEnabled(true);
    mapUi->modeComboBox->setCurrentIndex(pos.mode);
    mapUi->longitudeLineEdit->setText(QString::number(pos.static_position.longitude));
    mapUi->latitudeLineEdit->setText(QString::number(pos.static_position.latitude));
    mapUi->altitudeLineEdit->setText(QString::number(pos.static_position.altitude));
    mapUi->horErrorLineEdit->setText(QString::number(pos.static_position.hor_error));
    mapUi->vertErrorLineEdit->setText(QString::number(pos.static_position.vert_error));
    mapUi->maxDopLineEdit->setText(QString::number(pos.lock_in_max_dop));
    mapUi->minPosErrorLineEdit->setText(QString::number(pos.lock_in_min_error_meters));
}

void Map::onUiEnabledStateChange(bool connected)
{
    if (mapComponent == nullptr) {
        return;
    }
    QMetaObject::invokeMethod(mapComponent, "setEnabled",
        Q_ARG(QVariant, connected));
    mapUi->mapWidget->setEnabled(connected);
    if (!connected) {
        mapUi->positionModelGroupBox->setEnabled(false);
        mapUi->staticPositionGroupBox->setEnabled(false);
        mapUi->setConfigPushButton->setEnabled(false);
    }
}

void Map::on_setConfigPushButton_clicked()
{
    PositionModeConfig posconfig {};
    posconfig.mode = static_cast<PositionModeConfig::Mode>(mapUi->modeComboBox->currentIndex());
    bool ok { false };
    posconfig.static_position.longitude = mapUi->longitudeLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.static_position.latitude = mapUi->latitudeLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.static_position.altitude = mapUi->altitudeLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.static_position.hor_error = mapUi->horErrorLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.static_position.vert_error = mapUi->vertErrorLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.lock_in_max_dop = mapUi->maxDopLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    posconfig.lock_in_min_error_meters = mapUi->minPosErrorLineEdit->text().toDouble(&ok);
    if (!ok) {
        return;
    }
    emit posModeConfigChanged(posconfig);
}
