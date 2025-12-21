#include "SkyPrompt/API.hpp";

TESGlobal* hotkey;
std::vector<BGSKeyword*> allowedFood;
SkyPromptAPI::Prompt poisonPrompt{"$DBDUI_ADD", 0, 0, SkyPromptAPI::PromptType::kHint};
std::array<SkyPromptAPI::Prompt, 1> prompts = {poisonPrompt};
SkyPromptAPI::ClientID clientID;
std::pair<RE::INPUT_DEVICE, uint32_t> key{INPUT_DEVICE::kKeyboard, 0};

class MyPromptSink : public SkyPromptAPI::PromptSink {
public:
    std::span<const SkyPromptAPI::Prompt> GetPrompts() const override {
        return prompts;
    };

    void ProcessEvent(SkyPromptAPI::PromptEvent event) const override {
    };
};

static MyPromptSink g_PromptSink;

bool CanBePoisoned(TESBoundObject* form) {
    return form->HasKeywordInArray(allowedFood, false);
}

struct mySink : public RE::BSTEventSink<SKSE::CrosshairRefEvent> {
    RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* event,
                                          RE::BSTEventSource<SKSE::CrosshairRefEvent>* source) {
        static bool bShowing = false;
        if (bShowing) {
            SkyPromptAPI::RemovePrompt(&g_PromptSink, clientID);
            bShowing = false;
        }
        auto ref = event->crosshairRef;
        if (ref) {
            if (auto* base = ref ? ref->GetBaseObject() : nullptr) {
                if (base->GetFormType() == FormType::AlchemyItem && CanBePoisoned(base)) {
                    prompts[0].refid = ref->GetFormID();
                    key.second = (uint32_t)hotkey->value;
                    prompts[0].button_key = std::span{&key, 1};
                    SkyPromptAPI::SendPrompt(&g_PromptSink, clientID);
                    bShowing = true;
                }
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }
};

void setup() {
    hotkey = TESForm::LookupByEditorID<TESGlobal>("DBD_Hotkey");
    if (hotkey) {
        static mySink g_EventSink;
        SKSE::GetCrosshairRefEventSource()->AddEventSink(&g_EventSink);
        allowedFood.reserve(1);
        allowedFood.push_back(TESForm::LookupByEditorID<BGSKeyword>("DBD_Drink"));
        clientID = SkyPromptAPI::RequestClientID();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            setup();
        };
    });

    return true;
}