#pragma once
#include "product.hpp"
#include <cstdint>

namespace cardputer_adv {
// SD é a fonte persistente; chamadas reais passam por um worker.
class StorageBackend {
public:
    virtual ~StorageBackend() = default;
    virtual StorageState state() const = 0;
    // Barreira: resultado deve distinguir persistido de apenas enfileirado.
    virtual bool flush_pending() = 0;
    // Confirma uma geração validada e íntegra de configuração operacional.
    virtual bool commit_configuration(std::uint32_t generation) = 0;
};
} // namespace cardputer_adv
