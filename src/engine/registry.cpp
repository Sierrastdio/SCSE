#include "engine/world.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace spasim
{
namespace
{

template <typename T>
T *findRegistered(std::vector<std::optional<T>> &storage, const std::vector<std::optional<EntityRef>> &registry, EntityId id,
  EntityKind kind) noexcept
{
  if (id >= registry.size() || !registry[id] || registry[id]->kind != kind)
  {
    return nullptr;
  }
  const auto slot = registry[id]->slot;
  return slot < storage.size() && storage[slot] && storage[slot]->id == id ? &*storage[slot] : nullptr;
}

template <typename T>
const T *findRegistered(const std::vector<std::optional<T>> &storage, const std::vector<std::optional<EntityRef>> &registry,
  EntityId id, EntityKind kind) noexcept
{
  if (id >= registry.size() || !registry[id] || registry[id]->kind != kind)
  {
    return nullptr;
  }
  const auto slot = registry[id]->slot;
  return slot < storage.size() && storage[slot] && storage[slot]->id == id ? &*storage[slot] : nullptr;
}

template <typename T, typename AllocateId>
T *spawnInStorage(std::vector<std::optional<T>> &storage, T value, EntityKind kind, AllocateId &&allocateId)
{
  const auto slot = std::ranges::find_if(storage, [](const auto &entry) { return !entry; });
  const auto index = slot == storage.end() ? storage.size() : static_cast<std::size_t>(slot - storage.begin());
  if (slot == storage.end())
  {
    storage.emplace_back();
  }
  value.id = allocateId(kind, index);
  value.exists = true;
  storage[index] = std::move(value);
  return &*storage[index];
}

} // namespace

const Ship *World::findShip(EntityId id) const noexcept
{
  return findRegistered(ships_, entityRegistry_, id, EntityKind::Ship);
}

Ship *World::findShip(EntityId id) noexcept
{
  return findRegistered(ships_, entityRegistry_, id, EntityKind::Ship);
}

Base *World::findBase(EntityId id) noexcept
{
  return findRegistered(bases_, entityRegistry_, id, EntityKind::Base);
}

const Base *World::findBase(EntityId id) const noexcept
{
  return findRegistered(bases_, entityRegistry_, id, EntityKind::Base);
}

Torpedo *World::findTorpedo(EntityId id) noexcept
{
  return findRegistered(torpedoes_, entityRegistry_, id, EntityKind::Torpedo);
}

const Torpedo *World::findTorpedo(EntityId id) const noexcept
{
  return findRegistered(torpedoes_, entityRegistry_, id, EntityKind::Torpedo);
}

std::optional<EntityKind> World::entityKind(EntityId id) const noexcept
{
  return id < entityRegistry_.size() && entityRegistry_[id] ? std::optional<EntityKind>{entityRegistry_[id]->kind} : std::nullopt;
}

EntityId World::allocateEntityId(EntityKind kind, std::size_t slot)
{
  while (nextEntityId < entityRegistry_.size() && entityRegistry_[nextEntityId])
  {
    if (nextEntityId == std::numeric_limits<EntityId>::max())
    {
      throw std::overflow_error("Entity ID space exhausted.");
    }
    ++nextEntityId;
  }
  if (nextEntityId == std::numeric_limits<EntityId>::max())
  {
    throw std::overflow_error("Entity ID space exhausted.");
  }
  const EntityId id = nextEntityId++;
  if (entityRegistry_.size() <= id)
  {
    entityRegistry_.resize(static_cast<std::size_t>(id) + 1);
  }
  entityRegistry_[id] = EntityRef{kind, slot};
  return id;
}

void World::resetEntityStorage(std::size_t shipCapacity, std::size_t baseCapacity, std::size_t torpedoCapacity, EntityId firstId)
{
  playerSlots.clear();
  ships_.assign(shipCapacity, std::nullopt);
  bases_.assign(baseCapacity, std::nullopt);
  torpedoes_.assign(torpedoCapacity, std::nullopt);
  weaponProjectiles_.clear();
  entityRegistry_.clear();
  entityRegistry_.resize(firstId);
  nextEntityId = firstId;
}

void World::replaceProjectiles(std::vector<WeaponProjectile> projectiles)
{
  std::unordered_set<EntityId> retainedIds;
  retainedIds.reserve(projectiles.size());
  for (const auto &projectile : projectiles)
  {
    retainedIds.insert(projectile.id);
  }
  for (const auto &oldProjectile : weaponProjectiles_)
  {
    if (!retainedIds.contains(oldProjectile.id))
    {
      unregisterEntity(oldProjectile.id);
    }
  }
  weaponProjectiles_ = std::move(projectiles);
  for (std::size_t slot = 0; slot < weaponProjectiles_.size(); ++slot)
  {
    const EntityId id = weaponProjectiles_[slot].id;
    if (id < entityRegistry_.size() && entityRegistry_[id])
    {
      entityRegistry_[id]->slot = slot;
    }
  }
}

void World::unregisterEntity(EntityId id) noexcept
{
  if (id < entityRegistry_.size())
  {
    entityRegistry_[id].reset();
  }
}

Ship *World::spawnShip(Ship ship)
{
  return spawnInStorage(ships_, std::move(ship), EntityKind::Ship,
    [this](EntityKind kind, std::size_t slot) { return allocateEntityId(kind, slot); });
}

Base *World::spawnBase(Base base)
{
  return spawnInStorage(bases_, std::move(base), EntityKind::Base,
    [this](EntityKind kind, std::size_t slot) { return allocateEntityId(kind, slot); });
}

Torpedo *World::spawnTorpedo(Torpedo torpedo)
{
  return spawnInStorage(torpedoes_, std::move(torpedo), EntityKind::Torpedo,
    [this](EntityKind kind, std::size_t slot) { return allocateEntityId(kind, slot); });
}

WeaponProjectile *World::spawnWeaponProjectile(WeaponProjectile projectile)
{
  const auto index = weaponProjectiles_.size();
  projectile.id = allocateEntityId(EntityKind::WeaponProjectile, index);
  weaponProjectiles_.push_back(std::move(projectile));
  return &weaponProjectiles_.back();
}

bool World::despawn(EntityId id) noexcept
{
  if (id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return false;
  }
  const auto ref = *entityRegistry_[id];
  switch (ref.kind)
  {
    case EntityKind::Ship:
      if (ref.slot >= ships_.size() || !ships_[ref.slot] || ships_[ref.slot]->id != id)
      {
        return false;
      }
      ships_[ref.slot].reset();
      break;
    case EntityKind::Base:
      if (ref.slot >= bases_.size() || !bases_[ref.slot] || bases_[ref.slot]->id != id)
      {
        return false;
      }
      bases_[ref.slot].reset();
      break;
    case EntityKind::Torpedo:
      if (ref.slot >= torpedoes_.size() || !torpedoes_[ref.slot] || torpedoes_[ref.slot]->id != id)
      {
        return false;
      }
      torpedoes_[ref.slot].reset();
      break;
    case EntityKind::WeaponProjectile:
      if (ref.slot >= weaponProjectiles_.size() || weaponProjectiles_[ref.slot].id != id)
      {
        return false;
      }
      weaponProjectiles_.erase(weaponProjectiles_.begin() + static_cast<std::ptrdiff_t>(ref.slot));
      for (std::size_t i = ref.slot; i < weaponProjectiles_.size(); ++i)
      {
        entityRegistry_[weaponProjectiles_[i].id]->slot = i;
      }
      break;
  }
  unregisterEntity(id);
  return true;
}

WeaponProjectile *World::findWeaponProjectile(EntityId id) noexcept
{
  if (entityKind(id) != EntityKind::WeaponProjectile || id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return nullptr;
  }
  const auto slot = entityRegistry_[id]->slot;
  return slot < weaponProjectiles_.size() && weaponProjectiles_[slot].id == id ? &weaponProjectiles_[slot] : nullptr;
}

const WeaponProjectile *World::findWeaponProjectile(EntityId id) const noexcept
{
  if (entityKind(id) != EntityKind::WeaponProjectile || id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return nullptr;
  }
  const auto slot = entityRegistry_[id]->slot;
  return slot < weaponProjectiles_.size() && weaponProjectiles_[slot].id == id ? &weaponProjectiles_[slot] : nullptr;
}

} // namespace spasim
