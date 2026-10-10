#pragma once
#include <cstddef>
#include <cstdint>

namespace cardputer_adv {
// Contrato preliminar. Chamadas devem enfileirar trabalho, sem bloquear a UI.
// Estes nomes pertencem ao projeto, não são APIs verificadas do upstream.
enum class CommandResult : std::uint8_t {
    Queued, InvalidInput, NotReady, QueueFull, StorageUnavailable
};
struct ConversationRef {
    enum class Kind : std::uint8_t { Channel, Direct } kind;
    std::uint8_t channel_slot;
    std::uint32_t peer_node;
    // Identidade persistente da conversa será definida em STORAGE_SPEC.md.
};
class AppCommands {
public:
    virtual ~AppCommands() = default;
    virtual CommandResult send_text(ConversationRef target,
                                    const char* utf8, std::size_t bytes) = 0;
    virtual CommandResult review_channel_url(const char* url, std::size_t bytes) = 0;
};
} // namespace cardputer_adv
