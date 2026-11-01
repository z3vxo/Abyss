#include "../../includes/windows/login/login.h"


#include <QFile>
#include <QLineEdit>
#include <QPushButton>
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
