#pragma once

#include <QObject>
#include <QString>
#include <QWidget>
#include <qdialog.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwidget.h>


class QLineEdit;
class QPushButton;


class LoginWindow : public QObject {
    Q_OBJECT
public:
    explicit LoginWindow(QObject* parent = nullptr);
    ~LoginWindow();
    void show();
    void close();
    void setBusy(bool busy);

signals:
    void loginRequest(const QString& username, const QString& password,  const QString& domain, quint16 port);

private:
    void onDoneClicked();
    void loadProfiles();
    void saveProfile();
    QWidget *m_window = nullptr;
    QLineEdit* m_username = nullptr;
    QLineEdit* m_password = nullptr;
    QLineEdit* m_domain   = nullptr;
    QLineEdit* m_port     = nullptr;

    QPushButton* m_done = nullptr;


};
