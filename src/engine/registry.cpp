#include "engine/world.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace spasim
{

const Ship *World::findShip(EntityId id) const noexcept
{
  if (entityKind(id) != EntityKind::Ship || id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return nullptr;
  }
  const auto slot = entityRegistry_[id]->slot;
  return slot < ships_.size() && ships_[slot] && ships_[slot]->id == id ? &*ships_[slot] : nullptr;
}

Ship *World::findShip(EntityId id) noexcept
{
  if (entityKind(id) != EntityKind::Ship || id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return nullptr;
  }
  const auto slot = entityRegistry_[id]->slot;
  return slot < ships_.size() && ships_[slot] && ships_[slot]->id == id ? &*ships_[slot] : nullptr;
}

Base *World::findBase(EntityId id) noexcept
{
  if (entityKind(id) != EntityKind::Base || id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return nullptr;
  }
  const auto slot = entityRegistry_[id]->slot;
  return slot < bases_.size() && bases_[slot] && bases_[slot]->id == id ? &*bases_[slot] : nullptr;
}

const Base *World::findBase(EntityId id) const noexcept
{
  if (entityKind(id) != EntityKind::Base || id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return nullptr;
  }
  const auto slot = entityRegistry_[id]->slot;
  return slot < bases_.size() && bases_[slot] && bases_[slot]->id == id ? &*bases_[slot] : nullptr;
}

Torpedo *World::findTorpedo(EntityId id) noexcept
{
  if (entityKind(id) != EntityKind::Torpedo || id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return nullptr;
  }
  const auto slot = entityRegistry_[id]->slot;
  return slot < torpedoes_.size() && torpedoes_[slot] && torpedoes_[slot]->id == id ? &*torpedoes_[slot] : nullptr;
}

const Torpedo *World::findTorpedo(EntityId id) const noexcept
{
  if (entityKind(id) != EntityKind::Torpedo || id >= entityRegistry_.size() || !entityRegistry_[id])
  {
    return nullptr;
  }
  const auto slot = entityRegistry_[id]->slot;
  return slot < torpedoes_.size() && torpedoes_[slot] && torpedoes_[slot]->id == id ? &*torpedoes_[slot] : nullptr;
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
  for (const auto &oldProjectile : weaponProjectiles_)
  {
    const bool retained
      = std::ranges::any_of(projectiles, [&](const WeaponProjectile &projectile) { return projectile.id == oldProjectile.id; });
    if (!retained)
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
  const auto slot = std::ranges::find_if(ships_, [](const auto &entry) { return !entry; });
  const bool append = slot == ships_.end();
  if (append)
  {
    ships_.emplace_back();
  }
  const auto index = append ? ships_.size() - 1 : static_cast<std::size_t>(slot - ships_.begin());
  ship.id = allocateEntityId(EntityKind::Ship, index);
  ship.exists = true;
  ships_[index] = std::move(ship);
  return &*ships_[index];
}

Base *World::spawnBase(Base base)
{
  const auto slot = std::ranges::find_if(bases_, [](const auto &entry) { return !entry; });
  const bool append = slot == bases_.end();
  if (append)
  {
    bases_.emplace_back();
  }
  const auto index = append ? bases_.size() - 1 : static_cast<std::size_t>(slot - bases_.begin());
  base.id = allocateEntityId(EntityKind::Base, index);
  base.exists = true;
  bases_[index] = std::move(base);
  return &*bases_[index];
}

Torpedo *World::spawnTorpedo(Torpedo torpedo)
{
  const auto slot = std::ranges::find_if(torpedoes_, [](const auto &entry) { return !entry; });
  const bool append = slot == torpedoes_.end();
  if (append)
  {
    torpedoes_.emplace_back();
  }
  const auto index = append ? torpedoes_.size() - 1 : static_cast<std::size_t>(slot - torpedoes_.begin());
  torpedo.id = allocateEntityId(EntityKind::Torpedo, index);
  torpedo.exists = true;
  torpedoes_[index] = std::move(torpedo);
  return &*torpedoes_[index];
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
