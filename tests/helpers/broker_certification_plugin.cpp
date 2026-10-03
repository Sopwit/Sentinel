// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only native fixture exercising the existing ABI; never auto-installed.
#include "SentinelPluginSdk.h"
#include <memory>
#include <vector>
#include <QFile>
#ifdef Q_OS_MACOS
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
using namespace sentinel::plugin_sdk;
class BrokerTool final : public IPluginTool {
    IPluginContext* context_;
    QString operation_;
public:
    BrokerTool(IPluginContext* context,QString operation):context_(context),operation_(std::move(operation)){}
    PluginResult execute(const PluginInvocation& invocation) override {
        const auto& args=invocation.arguments;
#ifdef Q_OS_MACOS
        if(operation_=="containment") {
            const auto child=::fork();
            const bool forkDenied=child<0 && errno==EPERM;
            if(child==0)::_exit(0);
            if(child>0)::waitpid(child,nullptr,0);
            pid_t spawned=0;
            char program[]="/bin/true";char* argv[]={program,nullptr};char* environment[]={nullptr};
            const auto status=::posix_spawn(&spawned,program,nullptr,nullptr,argv,environment);
            if(status==0)::waitpid(spawned,nullptr,0);
            const int socket=::socket(AF_INET,SOCK_STREAM,0);
            sockaddr_in address{};address.sin_family=AF_INET;
            address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
            address.sin_port=htons(static_cast<uint16_t>(args.value("port").toInt()));
            const int connected=socket<0 ? -1 : ::connect(socket,reinterpret_cast<sockaddr*>(&address),sizeof(address));
            const bool networkDenied=connected<0 && (errno==EPERM || errno==EACCES);
            if(socket>=0)::close(socket);
            QFile file(args.value("path").toString());
            const bool fileDenied=!file.open(QIODevice::ReadOnly);
            return {forkDenied && status==EPERM && networkDenied && fileDenied,
                QString("forkDenied=%1 spawnDenied=%2 networkDenied=%3 fileDenied=%4")
                    .arg(forkDenied).arg(status==EPERM).arg(networkDenied).arg(fileDenied)};
        }
#endif
        if(operation_=="add")return {true,QString::number(args.value("a").toDouble()+args.value("b").toDouble())};
        if(operation_=="read") {
            const auto result=context_->filesystemRead({"path",args.value("path").toString()});
            return {result.ok,result.ok?QString::fromUtf8(result.content):result.category};
        }
        if(operation_=="process") {
            QStringList arguments;
            for(const auto& arg:args.value("arguments").toArray())arguments.append(arg.toString());
            const auto result=context_->processExecute({"program",args.value("program").toString(),"arguments",arguments,3000});
            return {result.ok,result.ok?QString::fromUtf8(result.stdoutData):result.category};
        }
        NetworkRequest request;request.urlArgument="url";request.url=args.value("url").toString();
        request.credentialId=args.value("credential").toString();request.timeoutMs=3000;
        const auto result=context_->networkRequest(request);
        return {result.ok,result.ok?QString::fromUtf8(result.body):result.category};
    }
};
class BrokerCertificationPlugin final : public QObject,public ISentinelPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ISentinelPlugin_iid FILE "../fixtures/plugins/broker/plugin.json")
    Q_INTERFACES(sentinel::plugin_sdk::ISentinelPlugin)
    PluginState state_=PluginState::Unloaded;
    std::vector<std::unique_ptr<BrokerTool>> tools_;
public:
    QString pluginId()const override{return "dev.sentinel.test.broker";}
    QString displayName()const override{return "Broker certification fixture";}
    QString vendor()const override{return "Sentinel tests";}
    QString version()const override{return "1.0.0";}
    QString requiredCoreVersion()const override{return ">=1.0.0";}
    bool initialize(IPluginContext* context)override {
        QStringList names{"add","read","process","network"};
#ifdef Q_OS_MACOS
        names.append("containment");
#endif
        for(const QString& name:names) {
            PluginToolDescriptor descriptor;descriptor.id=name;descriptor.name=name;
            descriptor.description="Deterministic existing-broker certification";
            QJsonObject properties;QJsonArray required;
            auto property=[&](const QString& key,const QString& type,bool mandatory=true) {
                properties.insert(key,QJsonObject{{"type",type}});if(mandatory)required.append(key);
            };
            auto authorization=[&](const QString& domain,const QString& access,const QString& kind,const QString& argument) {
                descriptor.authorizationRequirements.append(QJsonObject{{"domain",domain},{"access",access},
                    {"resourceKind",kind},{"resourceArgument",argument}});
            };
            if(name=="add") {property("a","number");property("b","number");}
            else if(name=="containment") {property("port","number");property("path","string");} // No broker grants.
            else if(name=="read") {
                property("path","string");authorization("filesystem","read","filesystem-path","path");
                descriptor.hostCapabilities={"FilesystemRead"};
            } else if(name=="process") {
                property("program","string");property("arguments","array");
                auto array=properties.value("arguments").toObject();array.insert("items",QJsonObject{{"type","string"}});
                properties.insert("arguments",array);
                authorization("process","execute","argument","program");descriptor.hostCapabilities={"ProcessExecute"};
            } else {
                property("url","string");property("credential","string",false);
                authorization("network","read","host","url");descriptor.hostCapabilities={"NetworkRequest"};
            }
            descriptor.inputSchema={{"type","object"},{"properties",properties},{"required",required},{"additionalProperties",false}};
            tools_.push_back(std::make_unique<BrokerTool>(context,name));
            if(!context->registerTool(descriptor,tools_.back().get()))return false;
        }
        state_=PluginState::Initialized;return true;
    }
    bool start()override{state_=PluginState::Active;return true;}
    void stop()override{state_=PluginState::Initialized;}
    void shutdown()override{tools_.clear();state_=PluginState::Unloaded;}
    PluginState state()const override{return state_;}
    QJsonObject defaultConfig()const override{return {};}
    void configure(const QJsonObject&)override{}
};
#include "broker_certification_plugin.moc"
