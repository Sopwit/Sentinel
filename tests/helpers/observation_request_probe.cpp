// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/agent/ObservationPolicy.h"
#include "sentinel/core/interfaces/IChatProvider.h"
#include "sentinel/core/runtime/LocalInference.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <cstdio>
using namespace sentinel::core;
class CaptureProvider final : public IChatProvider {
public:
    QString prompt;
    QString name() const override { return QStringLiteral("capture"); }
    ChatProviderStatus status() const override { return ChatProviderStatus::Ready; }
    ChatProviderReply sendMessage(const QString& text) override {
        prompt = text;
        return {true, QStringLiteral("{\"requirements\":[]}"), {}};
    }
};
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 5) {
        std::fprintf(stderr, "usage: probe MODEL MODE CANCEL_MS OUTPUT_JSON\nMODE: current|minimal|chat\n");
        return 2;
    }
    const auto goal = QStringLiteral("Bu workspace'in kök dizinindeki dosya ve klasörleri listele. Yalnızca gerçekten gözlemlediğin sonuçları kullan.");
    CaptureProvider capture;
    ObservationIntentPolicy policy(&capture);
    policy.classify(goal, {}, BuiltInToolProvider::descriptors());
    QString prompt = capture.prompt;
    if (args[2] == QLatin1String("minimal"))
        prompt = QStringLiteral("Classify whether the user goal requires current external observations. Return ONLY JSON {\"requirements\":[{\"domain\":\"filesystem|workspace|process|system|clipboard|network|browser|memory|history|external|application|execution\",\"resource\":\"\",\"purpose\":\"inspect|operate\",\"operation\":\"exists|contains|search_matches|none\",\"query\":\"\"}]}. At most four requirements. Use [] for conversation-only or writing tasks. GOAL: ") + goal;
    else if (args[2] == QLatin1String("chat"))
        prompt = QStringLiteral("Merhaba. Tek cümleyle yanıt ver.");
    else if (args[2] != QLatin1String("current")) return 2;
    const QJsonObject body{{"model", args[1]}, {"stream", false},
        {"messages", QJsonArray{QJsonObject{{"role", "user"}, {"content", prompt}}}}};
    auto token = std::make_shared<std::atomic_bool>(false);
    // A bounded experiment cancels; production request timeout remains zero.
    if (args[3].toInt() > 0)
        QTimer::singleShot(args[3].toInt(), &app, [token] { token->store(true); });
    QElapsedTimer elapsed;
    elapsed.start();
    LMStudioLocalInferenceClient client({}, 0);
    const auto result = client.completeOpenAiChat(body, token);
    const QJsonObject report{{"request", body}, {"prompt_characters", prompt.size()},
        {"input_token_estimate_chars_div_4", (prompt.size()+3)/4},
        {"elapsed_ms", elapsed.elapsed()}, {"ok", result.ok},
        {"cancelled", result.cancelled}, {"timed_out", result.timedOut},
        {"error", result.error}, {"response", result.body}};
    QFile output(args[4]);
    if (!output.open(QIODevice::WriteOnly)) return 3;
    output.write(QJsonDocument(report).toJson());
    return result.ok ? 0 : 1;
}
