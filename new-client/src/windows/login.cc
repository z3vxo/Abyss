#include "../../includes/windows/login/login.h"


#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSaveFile>
#include <QMessageBox>
#include <QUiLoader>
#include <qlineedit.h>
#include <qlogging.h>
#include <qmessagebox.h>
#include <qobject.h>
#include <qtypes.h>

LoginWindow::LoginWindow(QObject* parent) : QObject(parent) {
    QFile File("./resc/ui/login.ui");
	if(!File.open(QFile::ReadOnly)) {
	    qFatal("Failed to open login.ui");
		return;
	}

	QUiLoader loader;
	this->m_window = loader.load(&File);
	File.close();

	if(!this->m_window) {
	    qFatal("Failed to load login.ui");
		return;
	}

	this->m_username = this->m_window->findChild<QLineEdit*>("usernameEdit");
	this->m_password = this->m_window->findChild<QLineEdit*>("passwordEdit");
	this->m_domain   = this->m_window->findChild<QLineEdit*>("hostEdit");
	this->m_port     = this->m_window->findChild<QLineEdit*>("portEdit");
	this->m_done     = this->m_window->findChild<QPushButton*>("doneButton");


	if(!this->m_username || !this->m_password || !this->m_domain || !this->m_port || !this->m_done) {
	    qFatal("missing requested widgets in login");
		return;
	}

	connect(m_done, &QPushButton::clicked, this, &LoginWindow::onDoneClicked);
    auto* saveButton = m_window->findChild<QPushButton*>("saveProfileButton");
    auto* profiles = m_window->findChild<QListWidget*>("profilesList");
    auto* profileName = m_window->findChild<QLineEdit*>("profileNameEdit");
    if (!saveButton || !profiles || !profileName)
        qFatal("missing profile widgets in login");
    connect(saveButton, &QPushButton::clicked, this, &LoginWindow::saveProfile);
    connect(profiles, &QListWidget::itemClicked, this, [this, profileName](QListWidgetItem* item) {
        const QJsonObject profile = item->data(Qt::UserRole).toJsonObject();
        profileName->setText(profile.value("name").toString());
        m_domain->setText(profile.value("ip").toString());
        const QJsonValue port = profile.value("port");
        m_port->setText(port.isDouble() ? QString::number(port.toInt()) : port.toString());
        m_username->setText(profile.value("username").toString());
        m_password->setText(profile.value("password").toString());
    });
    loadProfiles();
}

void LoginWindow::loadProfiles() {
    auto* list = m_window->findChild<QListWidget*>("profilesList");
    auto* empty = m_window->findChild<QLabel*>("profilesEmpty");
    if (!list || !empty) {
        qWarning() << "Missing profiles widgets in login.ui";
        return;
    }

    list->clear();
    list->hide();
    empty->setText("No saved profiles");
    empty->show();
    QFile file("./resc/saved/profiles.json");
    if (!file.exists())
        return;
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Cannot read saved profiles:" << file.errorString();
        empty->setText("Could not load saved profiles");
        return;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) {
        qWarning() << "Invalid saved profiles JSON:" << error.errorString();
        empty->setText("Could not load saved profiles");
        return;
    }

    for (const QJsonValue& value : document.array()) {
        if (!value.isObject())
            continue;
        const QJsonObject profile = value.toObject();
        const QString name = profile.value("name").toString().trimmed();
        const QString ip = profile.value("ip").toString().trimmed();
        const QString username = profile.value("username").toString().trimmed();
        if (name.isEmpty() || ip.isEmpty() || username.isEmpty()) {
            qWarning() << "Skipping saved profile with missing name, ip or username";
            continue;
        }
        auto* item = new QListWidgetItem(name + "\n" + username + "  ·  " + ip, list);
        item->setData(Qt::UserRole, profile);
    }
    if (list->count() > 0) {
        empty->hide();
        list->show();
    }
}

void LoginWindow::saveProfile() {
    const QString name = m_window->findChild<QLineEdit*>("profileNameEdit")->text().trimmed();
    const QString ip = m_domain->text().trimmed();
    const QString username = m_username->text().trimmed();
    const QString password = m_password->text();
    bool validPort = false;
    const uint port = m_port->text().toUInt(&validPort);
    if (name.isEmpty() || ip.isEmpty() || username.isEmpty() || password.isEmpty()
        || !validPort || port == 0 || port > 65535) {
        QMessageBox::warning(m_window, "Save profile", "Enter a profile name and valid connection details.");
        return;
    }

    const QString path = "./resc/saved/profiles.json";
    QJsonArray profiles;
    QFile existing(path);
    if (existing.exists()) {
        if (!existing.open(QFile::ReadOnly)) {
            QMessageBox::warning(m_window, "Save profile", "Could not read saved profiles.");
            return;
        }
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(existing.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !document.isArray()) {
            QMessageBox::warning(m_window, "Save profile", "Saved profiles contain invalid JSON.");
            return;
        }
        profiles = document.array();
        for (const QJsonValue& entry : profiles) {
            if (entry.toObject().value("name").toString() == name) {
                QMessageBox::warning(m_window, "Save profile", "A profile with that name already exists.");
                return;
            }
        }
    }
    if (!QDir().mkpath("./resc/saved")) {
        QMessageBox::warning(m_window, "Save profile", "Could not create the profiles directory.");
        return;
    }
    profiles.append(QJsonObject{{"name", name}, {"ip", ip}, {"port", int(port)},
                                {"username", username}, {"password", password}});
    QSaveFile file(path);
    const QByteArray data = QJsonDocument(profiles).toJson();
    if (!file.open(QFile::WriteOnly) ||
        !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner) ||
        file.write(data) != data.size() || !file.commit()) {
        QMessageBox::warning(m_window, "Save profile", "Could not save the profile.");
        return;
    }
    loadProfiles();
}

LoginWindow::~LoginWindow() {
    delete m_window;
}

void LoginWindow::show() {
    this->m_window->show();
}

void LoginWindow::close() {
    this->m_window->close();
}

void LoginWindow::setBusy(bool busy) {
    this->m_done->setEnabled(!busy);
}

void LoginWindow::onDoneClicked() {
    const QString username = this->m_username->text();
    const QString password = this->m_password->text();
    const QString domain   = this->m_domain->text();
    bool validPort = false;
    const uint port        = this->m_port->text().toUInt(&validPort);
    if(username.isEmpty() || password.isEmpty() || domain.isEmpty()|| !validPort || port == 0 || port > 65535) {
        QMessageBox::warning(this->m_window, "invalid input", "please enter valid input");
        return;
    }
    this->m_done->setEnabled(false);

    emit loginRequest(username, password, domain, static_cast<quint16>(port));
}
