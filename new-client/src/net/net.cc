#include "net/net.h"


#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>


static ApiResult readResult(QNetworkReply* reply) {
	ApiResult r;
	r.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const QByteArray data = reply->readAll();

	if (reply->error() != QNetworkReply::NoError && r.status == 0) {
		r.error = reply->errorString();
		return r;
	}

	QJsonParseError perr;
	const auto doc = QJsonDocument::fromJson(data, &perr);
	if (doc.isObject()) r.body = doc.object();
	r.ok = r.status >= 200 && r.status < 300;
	if (!r.ok) {
		r.error = r.body.value("error").toString();
		if (r.error.isEmpty()) r.error = QString("HTTP %1").arg(r.status);
	}
	return r;
}

NetworkService::NetworkService(QObject* parent) : QObject(parent) {
	connect(&m_nam, &QNetworkAccessManager::sslErrors, this,
	        [](QNetworkReply* reply, const QList<QSslError>& errors) {
	            reply->ignoreSslErrors(errors);
	        });
}

void NetworkService::setEndpoint(const QString& domain, quint16 port) {
	m_base = QUrl(QString("https://%1:%2").arg(domain).arg(port));
}


void NetworkService::get(const QString& path, std::function<void(const ApiResult&)> cb) {
	QNetworkRequest req(m_base.resolved(QUrl(path)));
	req.setTransferTimeout(15000);
	if (!m_accessToken.isEmpty())
		req.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());

	QNetworkReply* reply = m_nam.get(req);

	connect(reply, &QNetworkReply::finished, reply, [reply, cb] {
		cb(readResult(reply));
		reply->deleteLater();
	});
}

void NetworkService::post(const QString& path, const QJsonObject& body,
                          std::function<void(const ApiResult&)> cb) {
	QNetworkRequest req(m_base.resolved(QUrl(path)));
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	req.setTransferTimeout(15000);
	if (!m_accessToken.isEmpty())
		req.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());

	QNetworkReply* reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

	connect(reply, &QNetworkReply::finished, reply, [reply, cb] {
		cb(readResult(reply));
		reply->deleteLater();
	});
}

void NetworkService::login(const QString& username, const QString& password,
                           std::function<void(const ApiResult&, const LoginResponse&)> cb) {
	const QJsonObject body{
		{"username", username},
		{"password", password},
	};

	post("/rest/auth/login", body, [this, cb](const ApiResult& r) {
		LoginResponse lr;
		if (r.ok) {
			lr.accessToken  = r.body.value("access").toString();
			lr.refreshToken = r.body.value("refresh").toString();
			m_accessToken   = lr.accessToken;
		}
		cb(r, lr);
	});
}


void NetworkService::NewTask(int32_t agentId, const QString& cmdName, const QVector<Arg>& args,
                             std::function<void(int, const QString&)> cb) {
    QJsonArray array;
    for(const auto& arg : args) {
        QJsonObject obj;
        obj["data_type"] = arg.Type;
        obj["arg"]  = arg.arg;
        array.append(obj);
    }

    QJsonObject json;
    json["agent_id"] = agentId;
    json["command"]  = cmdName;
    json["args"]     = array;

    this->post("/rest/tasks/new", json, [cb](const ApiResult& r) {
        cb(r.ok ? 0 : -1, r.error);
    });
}

void NetworkService::getAgents(std::function<void(const ApiResult&, const ListClientsResponse&)> cb) {
	get("/rest/agents/list", [cb](const ApiResult& r) {
		ListClientsResponse out;
		if (!r.ok) {
			cb(r, out);
			return;
		}

		out.total = r.body.value("total").toInt();
		const QJsonArray arr = r.body.value("agent").toArray();
		out.agents.reserve(arr.size());

		for (const QJsonValue& v : arr) {
			const QJsonObject o = v.toObject();
			AgentInfo a;
			a.agentId   = o.value("agent_id").toInt();
			a.agentName = o.value("agent_name").toString();
			a.username  = o.value("username").toString();
			a.hostname  = o.value("hostname").toString();
			a.domain    = o.value("domain").toString();
			a.procPath  = o.value("proc_path").toString();
			a.winVer    = o.value("winver").toString();
			a.pid       = o.value("pid").toInt();
			a.tid       = o.value("tid").toInt();
			a.ppid      = o.value("ppid").toInt();
			a.arch      = o.value("arch").toString();
			a.isAdmin   = o.value("is_admin").toString();
			a.lastSeen  = o.value("last_seen").toInt();
			a.createdAt = o.value("created_at").toInt();
			out.agents.append(a);
		}

		cb(r, out);
	});
}



void NetworkService::newListener(const NewListenerRequest& request,
                                 std::function<void(const ApiResult&)> cb) {
    const QJsonObject httpInfo{
        {"bind_addr", request.bind},
        {"port", request.port},
        {"http_paths", QJsonArray::fromStringList(request.paths)},
        {"tls_key", request.tlsKey},
        {"tls_cert", request.tlsCert}
    };
    const QJsonObject body{
        {"name", request.name},
        {"listener_type", request.type},
        {"http_listener_info", httpInfo},
        {"start_now", request.startNow}
    };
    post("/rest/listener/new", body, std::move(cb));
}
