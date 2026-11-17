# 엔진 변수 목록 트리

범위는 엔진 구현(`src/engine/`)과 엔진 규칙 구현(`src/rules/`)이다. 콘솔, 실행 진입점, 테스트 코드는 제외한다.

## 표기

- 구조체·클래스의 데이터 멤버는 선언한 타입별로 나눈다. 각 변수는 한 줄에 하나씩 적는다.
- enum 항목은 변수는 아니지만, 요청한 대로 해당 enum 아래에 따로 나열한다.
- 구현부에서는 함수별로 매개변수와 로컬 변수를 나눈다. 각 이름 한 줄에 짧은 사용 위치를 덧붙인다.
- 각 항목에는 C++ 자료형과 범주를 붙인다: `실수`, `정수`, `불리언`, `배열/컨테이너`, `구조체/클래스`, `enum`, `auto(추론)`.
- `detail.hpp`, `world.hpp`, `engine.hpp`는 별도 저장 변수가 없어 함수 선언 대신 실제 구현부에 매개변수를 적었다.

## 자료형별 데이터 멤버

### `src/engine/types.hpp`

#### `struct Vec3`

- `x` — `double` (실수); X 성분, 위치·방향·속도 계산에 사용.
- `y` — `double` (실수); Y 성분, 위치·방향·속도 계산에 사용.
- `z` — `double` (실수); Z 성분, 위치·방향·속도 계산에 사용.

#### `struct Orientation`

- `yawDegrees` — `double` (실수); 요 각도, `movement.cpp`, `weapon_aim.cpp`에서 회전·조준에 사용.
- `pitchDegrees` — `double` (실수); 피치 각도, `movement.cpp`, `weapon_aim.cpp`에서 회전·조준에 사용.

#### `enum class ObjectKind : std::uint8_t`

- `Ship` — `ObjectKind` enum 항목; `toString` 등 객체 종류 표시에 사용.
- `Base` — `ObjectKind` enum 항목; `toString` 등 객체 종류 표시에 사용.
- `Torpedo` — `ObjectKind` enum 항목; `toString` 등 객체 종류 표시에 사용.

#### `enum class AccelerationMode : std::uint8_t`

- `Decelerate` — `AccelerationMode` enum 항목; `movement.cpp`에서 속도 반대 방향으로 제동.
- `Cruise` — `AccelerationMode` enum 항목; `movement.cpp`에서 중력·관성 처리.
- `Accelerate` — `AccelerationMode` enum 항목; `movement.cpp`에서 함선 방향으로 추력.

#### `enum class ShipClass : std::uint8_t`

- `Small` — `ShipClass` enum 항목; `GameRules::maximumLevel`의 궤도 제한에 사용.
- `Medium` — `ShipClass` enum 항목; `GameRules::maximumLevel`의 궤도 제한에 사용.
- `Large` — `ShipClass` enum 항목; `GameRules::maximumLevel`의 궤도 제한에 사용.

#### `struct Resources`

- `energy` — `double` (실수); 에너지 잔량, `combat.cpp`, `economy.cpp`, `movement.cpp`에서 소비·이동.
- `rawMaterial` — `double` (실수); 원자재 잔량, `economy.cpp`의 적재·이동 처리.

#### `struct CombatState`

- `hull` — `double` (실수); 선체 최대치, 피해량·파괴 계산에 사용.
- `hullDamage` — `double` (실수); 누적 선체 피해, `combat.cpp`, `economy.cpp`에서 갱신.
- `shields` — `double` (실수); 방어막 잔량, `combat.cpp`에서 피해를 먼저 흡수.
- `skill` — `double` (실수); 전투 숙련도, 엔티티 전투 설정에 저장.
- `destroyed` — `bool` (불리언); 파괴 상태, 처리 제외 여부 판단.

#### `struct WeaponAimSettings`

- `maximumLeadSeconds` — `double` (실수); 최대 선행 조준 시간, `weapon_aim.cpp`에서 예측 시간 제한.
- `maximumHorizontalFiringAngleDegrees` — `double` (실수); 수평 발사각 한계, `weapon_aim.cpp`에서 발사 가능 여부 검사.
- `maximumVerticalFiringAngleDegrees` — `double` (실수); 수직 발사각 한계, `weapon_aim.cpp`에서 발사 가능 여부 검사.

#### `struct WeaponProfile`

- `name` — `std::string` (문자열); 무기 이름, 시나리오·상태 표시 데이터.
- `energyCost` — `double` (실수); 발사 에너지 비용, `combat.cpp`의 페이저 발사 처리.
- `damage` — `double` (실수); 무기 피해량, `combat.cpp`의 명중 처리.
- `range` — `double` (실수); 최대 사거리, `combat.cpp`의 발사 검사.
- `projectileSpeed` — `double` (실수); 투사체 속도, 조준·이동 처리.
- `explosive` — `bool` (불리언); 폭발 속성, `combat.cpp`에서 투사체로 전달.
- `splashRadius` — `double` (실수); 폭발 반경 설정.
- `aim` — `WeaponAimSettings` (구조체); 조준·발사각 검사에 사용.

#### `struct Ship`

- `id` — `EntityId` (`uint32_t`, 정수); 월드 검색·명령 처리.
- `owner` — `PlayerId` (`uint32_t`, 정수); 소유권 검사.
- `team` — `TeamId` (`uint32_t`, 정수); 동맹·피해 검사.
- `name` — `std::string` (문자열); 함선 이름·표시.
- `shipClass` — `ShipClass` (enum); 궤도 단계 제한.
- `position` — `Vec3` (구조체); 위치·거리 계산.
- `velocity` — `Vec3` (구조체); 운동·조준 계산.
- `orientation` — `Orientation` (구조체); 회전·발사각 검사.
- `desiredOrientation` — `Orientation` (구조체); 목표 방향 조향.
- `turnRateDegreesPerSecond` — `double` (실수); 방향 갱신.
- `speed` — `double` (실수); 현재 속력 갱신.
- `warp` — `double` (실수); 호환 속도 상태.
- `acceleration` — `AccelerationMode` (enum); 비행 동작 결정.
- `throttleLevel` — `uint8_t` (정수); 속도 제한 계산.
- `resources` — `Resources` (구조체); 자원 처리.
- `combat` — `CombatState` (구조체); 피해·파괴 상태 처리.
- `weapon` — `WeaponId` (`uint32_t`, 정수); 무기 선택·조준.
- `collisionRadius` — `double` (실수); 충돌 계산.
- `orbitingPlanet` — `std::optional<PlanetId>` (선택적 정수); 현재 궤도 행성.
- `orbitLevel` — `uint8_t` (정수); 현재 궤도 단계.
- `orbitTransferTargetLevel` — `std::optional<uint8_t>` (선택적 정수); 전이 목표 단계.
- `goalPlanet` — `std::optional<PlanetId>` (선택적 정수); 항법 목표 행성.
- `goalOrbitLevel` — `uint8_t` (정수); 항법 목표 궤도 단계.
- `selectedTarget` — `std::optional<EntityId>` (선택적 정수); 조준 대상.
- `torpedoId` — `EntityId` (`uint32_t`, 정수); 어뢰 검색 ID.
- `playerControlled` — `bool` (불리언); 제어 대상 필터링.
- `exists` — `bool` (불리언); 유효성 검사.

#### `struct Base`

- `id` — `EntityId` (`uint32_t`, 정수); 월드 검색.
- `owner` — `PlayerId` (`uint32_t`, 정수); 기지 소유자.
- `team` — `TeamId` (`uint32_t`, 정수); 동맹 검사.
- `homePlanet` — `PlanetId` (`uint8_t`, 정수); 모행성.
- `position` — `Vec3` (구조체); 위치·거리 검사.
- `resources` — `Resources` (구조체); 자원 이동.
- `combat` — `CombatState` (구조체); 피해 적용.
- `collisionRadius` — `double` (실수); 투사체 명중 계산.
- `exists` — `bool` (불리언); 월드 검색·충돌 유효성 검사.

#### `struct Torpedo`

- `id` — `EntityId` (`uint32_t`, 정수); 월드 검색·선택.
- `owner` — `PlayerId` (`uint32_t`, 정수); 자기 어뢰 충돌 방지.
- `team` — `TeamId` (`uint32_t`, 정수); 동맹 검사.
- `position` — `Vec3` (구조체); 매 틱 위치 갱신.
- `velocity` — `Vec3` (구조체); 중력 반영·이동.
- `energy` — `double` (실수); 피격 시 감소.
- `warp` — `double` (실수); 속도 호환 상태.
- `fieldStrength` — `double` (실수); 발사 에너지에서 설정.
- `damage` — `double` (실수); 폭발 피해량.
- `lifeSeconds` — `double` (실수); 만료 검사.
- `collisionRadius` — `double` (실수); 행성·함선 충돌 검사.
- `controlled` — `bool` (불리언); 제어 상태.
- `detonated` — `bool` (불리언); 중복 폭발 방지.
- `exists` — `bool` (불리언); 업데이트·검색 유효성 검사.

#### `struct WeaponProjectile`

- `id` — `EntityId` (`uint32_t`, 정수); 생성 시 월드 ID 발급.
- `shooter` — `EntityId` (`uint32_t`, 정수); 발사 함선 ID.
- `target` — `EntityId` (`uint32_t`, 정수); 목표 추적·명중 처리.
- `team` — `TeamId` (`uint32_t`, 정수); 동맹 피해 검사.
- `position` — `Vec3` (구조체); 투사체 위치 갱신.
- `velocity` — `Vec3` (구조체); 위치 계산.
- `speed` — `double` (실수); 이동 시간·사거리 계산.
- `damage` — `double` (실수); 명중 처리.
- `collisionRadius` — `double` (실수); 명중 검사.
- `remainingRange` — `double` (실수); 만료 검사.
- `explosive` — `bool` (불리언); 폭발 속성.

#### `struct Target`

- `kind` — `Target::Kind` (enum); 엔티티·행성 분기.
- `id` — `uint32_t` (정수); 월드에서 대상 검색.

#### `enum class Target::Kind` (기본 기반형 `int`)

- `Entity` — `Target::Kind` enum 항목; 전투 조준·선택 명령에서 사용.
- `Planet` — `Target::Kind` enum 항목; 항법 목표에서 사용.

#### `struct Player`

- `id` — `PlayerId` (`uint32_t`, 정수); 월드 검색·명령 인증.
- `team` — `TeamId` (`uint32_t`, 정수); 동맹·팀 데이터 접근.
- `callSign` — `std::string` (문자열); 플레이어 함선 이름.
- `station` — `std::string` (문자열); 플레이어 정거장 데이터.
- `shipId` — `EntityId` (`uint32_t`, 정수); 명령 대상 검색.
- `baseId` — `EntityId` (`uint32_t`, 정수); 수리·기지 접근.
- `selectedObject` — `EntityId` (`uint32_t`, 정수); 선택 객체 갱신.
- `goal` — `std::optional<Target>` (선택적 구조체); 항법 목표.
- `goalOrbitLevel` — `uint8_t` (정수); 목표 궤도 단계.
- `phasorPower` — `double` (실수); 기본 페이저 출력.
- `phasorDiameter` — `double` (실수); 투사체 크기 설정.
- `weapon` — `WeaponId` (`uint32_t`, 정수); 기본 무기 ID.
- `selectedShips` — `std::vector<EntityId>` (배열/컨테이너); 다중 함선 명령.
- `phasorEnabled` — `bool` (불리언); 페이저 활성 상태.
- `joined` — `bool` (불리언); 팀 인원수 계산.

#### `struct Planet`

- `id` — `PlanetId` (`uint8_t`, 정수); 행성 검색.
- `name` — `std::string` (문자열); 시나리오·상태 표시.
- `position` — `Vec3` (구조체); 중력·거리·항법 계산.
- `radius` — `double` (실수); 표면 충돌.
- `gravitationalParameter` — `double` (실수); 중력·궤도 계산.
- `parkingOrbitAltitude` — `double` (실수); 궤도 반경 계산.
- `sphereOfInfluenceRadius` — `double` (실수); 포획·속도 제한 계산.
- `orbitLevelCount` — `uint8_t` (정수); 궤도 단계 범위 확인.
- `orbitLevelRadii` — `std::array<double, kMaxOrbitLevels>` (고정 배열); 궤도 위치 계산.
- `population` — `double` (실수); 행성 경제 데이터.
- `antiEntropy` — `double` (실수); 적재 가능한 에너지 재고.
- `rawMaterial` — `double` (실수); 적재 가능한 원자재 재고.
- `standardOfLiving` — `double` (실수); 행성 경제 데이터.
- `skill` — `double` (실수); 행성 기술 수준.
- `controllingTeam` — `std::optional<TeamId>` (선택적 정수); 지배 팀.
- `revoltAtSeconds` — `double` (실수); 반란 기준 시간.

#### `struct Team`

- `id` — `TeamId` (`uint32_t`, 정수); 팀 목록 식별.
- `homePlanet` — `PlanetId` (`uint8_t`, 정수); 함선·기지 생성.
- `shipClass` — `ShipClass` (enum); 함선 초기화.
- `name` — `std::string` (문자열); 상태·스냅샷 표시.
- `spawnOffset` — `Vec3` (구조체); 시작 위치 계산.
- `baseOffset` — `Vec3` (구조체); 기지 생성 위치 계산.
- `spawnRadius` — `double` (실수); 무작위 생성 반경.
- `spawnVerticalMinimum` — `double` (실수); 방향 난수 최솟값.
- `spawnVerticalMaximum` — `double` (실수); 방향 난수 최댓값.
- `startingOrbitLevel` — `uint8_t` (정수); 초기 궤도 설정.
- `randomSpawn` — `bool` (불리언); 생성 위치 분기.
- `allies` — `std::vector<TeamId>` (배열/컨테이너); 동맹 조회·갱신.

#### `struct World`

- `teams` — `std::vector<Team>` (배열/컨테이너); 팀 설정·동맹 접근.
- `planets` — `std::vector<Planet>` (배열/컨테이너); 이동·항법·중력 계산.
- `playerSlots` — `std::vector<std::optional<Player>>` (배열/컨테이너); 플레이어 생성·검색.
- `ships` — `std::vector<std::optional<Ship>>` (배열/컨테이너); 이동·전투 처리.
- `bases` — `std::vector<std::optional<Base>>` (배열/컨테이너); 경제·전투 처리.
- `torpedoes` — `std::vector<std::optional<Torpedo>>` (배열/컨테이너); 발사·업데이트.
- `weaponProjectiles` — `std::vector<WeaponProjectile>` (배열/컨테이너); 투사체 생성·업데이트.
- `playersPerTeam` — `std::size_t` (정수); 참가 제한.
- `shipsPerPlayer` — `std::size_t` (정수); 슬롯·ID 할당.
- `tick` — `std::uint64_t` (정수); 매 스텝 증가.
- `elapsedSeconds` — `double` (실수); 누적 시뮬레이션 시간.
- `fixedStepSeconds` — `double` (실수); 이동·전투 업데이트 간격.
- `random` — `std::mt19937_64` (클래스); 무작위 생성.
- `randomSeedOverride` — `std::optional<std::uint64_t>` (선택적 정수); 난수 시드 덮어쓰기.
- `nextEntityId` — `EntityId` (`uint32_t`, 정수); 새 엔티티 ID 발급.
- `entityIdStart` — `EntityId` (`uint32_t`, 정수); 초기 엔티티 ID.
- `playerIdStart` — `PlayerId` (`uint32_t`, 정수); 플레이어 ID 시작점.
- `defaultWeapon` — `WeaponId` (`uint32_t`, 정수); 기본 무기 선택.
- `friendlyFire` — `bool` (불리언); 아군 피해 검사.
- `paused` — `bool` (불리언); 일시정지 상태.
- `sessionDurationSeconds` — `double` (실수); 세션 제한 시간.
- `weapons` — `std::vector<WeaponProfile>` (배열/컨테이너); 발사·조준 처리.
- `playerDefaults` — `Player` (구조체); 플레이어 생성 기본값.
- `playerShipDefaults` — `Ship` (구조체); 플레이어 함선 생성 기본값.
- `practiceShipDefaults` — `Ship` (구조체); 연습 함선 생성 기본값.
- `baseDefaults` — `Base` (구조체); 기지 생성 기본값.
- `torpedoLaunchSpeed` — `double` (실수); 어뢰 초기 속도.
- `torpedoLifeSeconds` — `double` (실수); 어뢰 기본 수명.
- `torpedoCollisionRadius` — `double` (실수); 어뢰 충돌 반경 기본값.

### `src/engine/actions.hpp`

#### 명령 구조체

- `SetHeading::direction` — `Vec3` (구조체); `Simulation::execute`에서 항법 처리기로 전달.
- `SetAcceleration::mode` — `AccelerationMode` (enum); `navigation.cpp` 처리기로 전달.
- `SetThrottleLevel::level` — `std::uint8_t` (정수); `navigation.cpp` 처리기로 전달.
- `SetGoal::target` — `Target` (구조체); 항법 처리기로 전달.
- `SetGoal::orbitLevel` — `std::uint8_t` (정수); 단계 검증·설정.
- `SetOrbitLevel::level` — `std::uint8_t` (정수); 궤도 전이 처리.
- `SetShipClass::shipClass` — `ShipClass` (enum); 함급 갱신.
- `FirePhasor::power` — `double` (실수); 비용·피해 계산.
- `SelectTarget::target` — `Target` (구조체); 조준 설정.
- `SelectShips::ships` — `std::vector<EntityId>` (배열/컨테이너); 소유권 확인.
- `FireTorpedo::energy` — `double` (실수); 에너지 소비·피해 설정.
- `FireTorpedo::aim` — `Vec3` (구조체); 어뢰 초기 속도 설정.
- `FireTorpedo::useShipFacing` — `bool` (불리언); 발사 방향 선택.
- `SetWeaponProfile::weapon` — `WeaponId` (`uint32_t`, 정수); 선택 무기 갱신.
- `RepairVessel::amount` — `double` (실수); 수리량 계산.
- `TransferResources::target` — `Target` (구조체); 자원 수신 엔티티 검색.
- `TransferResources::energy` — `bool` (불리언); 에너지/원자재 선택.
- `TransferResources::amount` — `double` (실수); 자원 이동량 계산.
- `LoadFromPlanet::planet` — `PlanetId` (`uint8_t`, 정수); 적재 행성 검색.
- `LoadFromPlanet::energy` — `bool` (불리언); 에너지/원자재 선택.
- `LoadFromPlanet::amount` — `double` (실수); 적재량 계산.
- `SetAlliances::first` — `TeamId` (`uint32_t`, 정수); 첫 팀 ID.
- `SetAlliances::second` — `TeamId` (`uint32_t`, 정수); 두 번째 팀 ID.
- `SetAlliances::allied` — `bool` (불리언); 동맹 추가·해제 분기.
- `DetonateTorpedo` — 데이터 멤버 없음; `combat.cpp`에서 어뢰 폭발 명령으로 사용.
- `TickWorld` — 데이터 멤버 없음; `engine.cpp`에서 한 틱 진행 명령으로 사용.

#### `enum class CommandStatus : std::uint8_t`

- `Applied` — `CommandStatus` enum 항목; 성공 판정에 사용.
- `Invalid` — `CommandStatus` enum 항목; 잘못된 입력·상태 반환.
- `NotFound` — `CommandStatus` enum 항목; 검색 실패 반환.
- `Unsupported` — `CommandStatus` enum 항목; 미지원 상태.

#### `struct CommandResult`

- `status` — `CommandStatus` (enum); 호출자가 성공 여부 확인.
- `message` — `std::string` (문자열); 프런트엔드 출력 메시지.

`WorldCommand`는 위 명령 타입들의 `std::variant` 별칭이며 멤버 변수는 없다.

### `src/rules/game_rules.hpp`

#### `struct PracticeShipDefinition`

- `team` — `TeamId` (`uint32_t`, 정수); `GameRules::applyScenario`에서 함선 생성.
- `name` — `std::string` (문자열); 생성된 함선에 복사.
- `position` — `Vec3` (구조체); 생성된 함선에 복사.

#### `class GameRules`

- `sphereOfInfluenceRadius` — `double` (실수); 궤도·비행 계산 설정.
- `transferMaximumRadialSpeed` — `double` (실수); 궤도 전이 최대 방사 속도.
- `transferRadialAcceleration` — `double` (실수); 궤도 전이 가속도.
- `levelCount` — `std::uint8_t` (정수); 궤도 단계 수.
- `maximumLevelByClass` — `std::array<std::uint8_t, 3>` (고정 배열); 함급별 최대 궤도 단계.
- `radiusFractions` — `std::array<double, kMaxOrbitLevels>` (고정 배열); 단계별 반지름 비율.
- `throttleSpeedMultipliers` — `std::array<double, 4>` (고정 배열); 스로틀별 속도 배수.
- `referenceGravitationalParameter` — `double` (실수); 속도 제한 기준 중력계수.
- `thrustAcceleration` — `double` (실수); 추진 가속도.
- `brakingAcceleration` — `double` (실수); 제동 가속도.
- `randomSeed` — `std::uint64_t` (정수); 월드 난수 엔진 초기화.
- `fixedStepSeconds` — `double` (실수); `World`에 복사되는 고정 간격.
- `sessionDurationSeconds` — `double` (실수); 세션 시간 설정.
- `friendlyFire` — `bool` (불리언); 아군 사격 설정.
- `paused` — `bool` (불리언); 초기 일시정지 설정.
- `playerDefaults` — `Player` (구조체); 기본 플레이어 상태.
- `playerShipDefaults` — `Ship` (구조체); 기본 플레이어 함선 상태.
- `practiceShipDefaults` — `Ship` (구조체); 기본 연습 함선 상태.
- `baseDefaults` — `Base` (구조체); 기본 기지 상태.
- `torpedoLaunchSpeed` — `double` (실수); 어뢰 발사 속도.
- `torpedoLifeSeconds` — `double` (실수); 어뢰 수명.
- `torpedoCollisionRadius` — `double` (실수); 어뢰 충돌 반경.
- `firstEntityId` — `EntityId` (`uint32_t`, 정수); 초기 ID 발급 상태.
- `firstPlayerId` — `PlayerId` (`uint32_t`, 정수); 플레이어 ID 시작점.
- `defaultWeapon` — `WeaponId` (`uint32_t`, 정수); 기본 무기.
- `playersPerTeam` — `std::size_t` (정수); 팀별 인원 제한.
- `shipsPerPlayer` — `std::size_t` (정수); 플레이어별 함선 수.
- `teams` — `std::vector<Team>` (배열/컨테이너); 월드에 팀 정의 복사.
- `planets` — `std::vector<Planet>` (배열/컨테이너); 월드에 행성 정의 복사.
- `weapons` — `std::vector<WeaponProfile>` (배열/컨테이너); 월드에 무기 정의 복사.
- `practiceShips` — `std::vector<PracticeShipDefinition>` (배열/컨테이너); 월드에 연습 함선 생성.

### `src/engine/engine.hpp`

#### `class Simulation`

- `world_` — `World` (구조체); `step`, `execute`와 엔진 서브시스템이 읽고 갱신.
- `gameRules_` — `GameRules` (클래스); 명령·이동 계산에 전달.

### `src/engine/types.hpp`의 상수

- `kMaxOrbitLevels` — `constexpr std::size_t` (정수 상수); `Planet`, `GameRules` 고정 배열 길이.
- `kNoPlayer` — `constexpr PlayerId` (`uint32_t`, 정수 상수); 소유자 미할당 상태.

## 구현부별 매개변수와 로컬 변수

각 항목은 `함수명 → 분류 → 변수: 간단한 용도` 순서다. 매개변수는 호출로 전달되고, 로컬 변수는 해당 함수 안에서만 쓰인다.

### `src/engine/math.cpp`

#### `forwardVector`

- 매개변수 `orientation` — `const Orientation&` (구조체 참조); 방향 각도 입력.
- 로컬 `toRadians` — `constexpr double` (실수 상수); 각도 단위 변환.
- 로컬 `yaw` — `const double` (실수); 라디안 단위 요 각도.
- 로컬 `pitch` — `const double` (실수); 라디안 단위 피치 각도.

#### `orientationToward`

- 매개변수 `direction` — `const Vec3&` (구조체 참조); 바라볼 방향 벡터.

#### `combatState`

- 매개변수 `world` — `World&` (구조체 참조); 엔티티 검색 대상 월드.
- 매개변수 `target` — `Target` (구조체 값); 전투 상태를 찾을 대상.
- 로컬 `ship` — `auto*` (추론 포인터); 함선 검색 결과.
- 로컬 `base` — `auto*` (추론 포인터); 기지 검색 결과.

#### `teamOf`

- 매개변수 `world` — `const World&` (구조체 참조); 엔티티 검색 대상 월드.
- 매개변수 `id` — `EntityId` (`uint32_t`, 정수); 검색 ID.
- 로컬 `ship` — `const auto*` (추론 포인터); 함선 검색 결과.
- 로컬 `base` — `const auto*` (추론 포인터); 기지 검색 결과.
- 로컬 `torpedo` — `const auto*` (추론 포인터); 어뢰 검색 결과.

#### `positionOf`

- 매개변수 `world` — `World&` (구조체 참조); 위치를 검색할 월드.
- 매개변수 `target` — `Target` (구조체 값); 위치를 조회할 대상.
- 로컬 `ship` — `auto*` (추론 포인터); 함선 검색 결과.
- 로컬 `base` — `auto*` (추론 포인터); 기지 검색 결과.
- 로컬 `torpedo` — `auto*` (추론 포인터); 어뢰 검색 결과.

### `src/engine/world.cpp`

#### `Vec3` 연산자와 거리 함수

- 매개변수 `other` — `const Vec3&` (구조체 참조); `distanceTo`의 비교 벡터.
- 매개변수 `other` — `const Vec3&` (구조체 참조); `operator+=`의 덧셈 벡터.
- 매개변수 `other` — `const Vec3&` (구조체 참조); `operator-=`의 뺄셈 벡터.
- 매개변수 `lhs` — `Vec3` (구조체 값); `operator+`·`operator-` 왼쪽 값.
- 매개변수 `rhs` — `const Vec3&` (구조체 참조); `operator+`·`operator-` 오른쪽 값.
- 매개변수 `value` — `Vec3` (구조체 값); 스칼라 곱 대상.
- 매개변수 `scalar` — `double` (실수); 벡터 성분에 곱할 수.

#### `World::findPlayer` (const/non-const)

- 매개변수 `id` — `PlayerId` (`uint32_t`, 정수); 찾을 플레이어 ID.
- 로컬 `found` — `auto` (추론 반복자); 일치한 플레이어 슬롯.
- 람다 매개변수 `slot` — `const auto&` (추론 참조); 검사 중인 플레이어 슬롯.

#### `World::findShip` (const/non-const)

- 매개변수 `id` — `EntityId` (`uint32_t`, 정수); 찾을 함선 ID.
- 로컬 `found` — `auto` (추론 반복자); 일치한 함선 슬롯.
- 람다 매개변수 `slot` — `const auto&` (추론 참조); 검사 중인 함선 슬롯.

#### `World::findBase` (const/non-const)

- 매개변수 `id` — `EntityId` (`uint32_t`, 정수); 찾을 기지 ID.
- 로컬 `found` — `auto` (추론 반복자); 일치한 기지 슬롯.
- 람다 매개변수 `slot` — `const auto&` (추론 참조); 검사 중인 기지 슬롯.

#### `World::findTorpedo` (const/non-const)

- 매개변수 `id` — `EntityId` (`uint32_t`, 정수); 찾을 어뢰 ID.
- 로컬 `found` — `auto` (추론 반복자); 일치한 어뢰 슬롯.
- 람다 매개변수 `slot` — `const auto&` (추론 참조); 검사 중인 어뢰 슬롯.

#### `World::activePlayerCount`

- 매개변수 `team` — `TeamId` (`uint32_t`, 정수); 인원을 셀 팀 ID.
- 람다 매개변수 `slot` — `const auto&` (추론 참조); 참가 상태 확인.

#### `toString` 오버로드

- 매개변수 `value` — `ObjectKind` (enum); 문자열화할 객체 종류.
- 매개변수 `value` — `AccelerationMode` (enum); 문자열화할 가속 모드.
- 매개변수 `value` — `ShipClass` (enum); 문자열화할 함급.

#### `makeInitialWorld`

- 매개변수 `seed` — `std::optional<std::uint64_t>` (선택적 정수); 난수 시드.
- 로컬 `world` — `World` (구조체); 생성해 반환할 월드.

#### `joinTeam`

- 매개변수 `world` — `World&` (구조체 참조); 참가시킬 월드.
- 매개변수 `team` — `TeamId` (`uint32_t`, 정수); 참가할 팀 ID.
- 매개변수 `callSign` — `std::string` (문자열); 새 플레이어 호출명.
- 로컬 `playerSlot` — `auto` (추론 반복자); 비어 있는 플레이어 슬롯.
- 로컬 `baseSlot` — `auto` (추론 반복자); 비어 있는 기지 슬롯.
- 로컬 `shipSlots` — `std::vector<std::optional<Ship>*>` (배열/컨테이너); 빈 함선 슬롯.
- 로컬 `torpedoSlots` — `std::vector<std::optional<Torpedo>*>` (배열/컨테이너); 빈 어뢰 슬롯.
- 로컬 `slot` — `auto&` (추론 참조); 슬롯 탐색·반복 원소.
- 로컬 `playerIndex` — `std::size_t` (정수); 플레이어 슬롯 인덱스.
- 로컬 `id` — `PlayerId` (정수); 새 플레이어 ID.
- 로컬 `planetId` — `const auto` (추론 정수); 팀의 모행성 ID.
- 로컬 `teamSettings` — `const Team&` (구조체 참조); 팀 설정.
- 로컬 `vertical` — `std::uniform_real_distribution<double>` (분포 객체); 수직 성분 난수.
- 로컬 `azimuth` — `std::uniform_real_distribution<double>` (분포 객체); 방위각 난수.
- 로컬 `spawnOffset` — `Vec3` (구조체); 생성 오프셋.
- 로컬 `z` — `const double` (실수); 난수 방향의 Z 성분.
- 로컬 `angle` — `const double` (실수); 방위각 또는 궤도 간격 각도.
- 로컬 `planar` — `const double` (실수); XY 평면 성분 크기.
- 로컬 `spawnRadius` — `const double` (실수); 생성 오프셋 길이.
- 로컬 `spawnDirection` — `const Vec3` (구조체); 시작 단위 방향.
- 로컬 `startOrbit` — `const auto` (추론 정수); 시작 궤도 단계.
- 로컬 `spawn` — `const Vec3` (구조체); 생성 위치.
- 로컬 `player` — `Player` (구조체); 새 플레이어 객체.
- 로컬 `shipIds` — `std::vector<EntityId>` (배열/컨테이너); 함선 ID 목록.
- 로컬 `torpedoIds` — `std::vector<EntityId>` (배열/컨테이너); 어뢰 ID 목록.
- 로컬 `index` — `std::size_t` (정수); 함선 생성 인덱스.
- 로컬 `axis` — `const Vec3` (구조체); 접선 계산 기준축.
- 로컬 `tangent` — `Vec3` (구조체); 시작 궤도 접선.
- 로컬 `orbitRadius` — `const double` (실수); 시작 궤도 반지름.
- 로컬 `orbitSpeed` — `const double` (실수); 시작 원궤도 속도.
- 로컬 `ship` — `Ship` (구조체); 초기화 중인 함선.
- 로컬 `radial` — `const Vec3` (구조체); 함선별 반경 방향.
- 로컬 `shipTangent` — `Vec3` (구조체); 함선별 접선 방향.
- 로컬 `torpedo` — `Torpedo` (구조체); 함선에 대응하는 어뢰.
- 로컬 `base` — `Base` (구조체); 새 기지 객체.

#### `removePlayer`

- 매개변수 `world` — `World&` (구조체 참조); 제거할 월드.
- 매개변수 `playerId` — `PlayerId` (`uint32_t`, 정수); 제거 대상 ID.
- 로컬 `found` — `auto` (추론 반복자); 플레이어 슬롯 검색 결과.
- 로컬 `slot` — `auto&` (추론 참조); 플레이어·함선·기지·어뢰 슬롯.
- 로컬 `player` — `const Player` (구조체); 제거 전 플레이어 복사본.

### `src/engine/navigation.cpp`

#### `executeAction(SetHeading)`

- 매개변수 `player` — `Player&` (구조체 참조); 플레이어 항법 상태 갱신.
- 매개변수 `ship` — `Ship&` (구조체 참조); 방향·궤도·대상 갱신.
- 매개변수 `action` — `const SetHeading&` (명령 구조체 참조); 조향 방향.

#### `executeAction(SetAcceleration)`

- 매개변수 `ship` — `Ship&` (구조체 참조); 가속·궤도 상태 갱신.
- 매개변수 `action` — `const SetAcceleration&` (명령 구조체 참조); 가속 모드.
- 로컬 `wasTransferringOrbit` — `const bool` (불리언); 기존 궤도 전이 여부.

#### `executeAction(SetThrottleLevel)`

- 매개변수 `ship` — `Ship&` (구조체 참조); 스로틀 갱신 대상.
- 매개변수 `action` — `const SetThrottleLevel&` (명령 구조체 참조); 새 단계.

#### `executeAction(SetGoal)`

- 매개변수 `world` — `World&` (구조체 참조); 목표 행성 검색.
- 매개변수 `player` — `Player&` (구조체 참조); 플레이어 목표 갱신.
- 매개변수 `ship` — `Ship&` (구조체 참조); 자동 항법 대상.
- 매개변수 `action` — `const SetGoal&` (명령 구조체 참조); 행성·궤도 목표.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 함급 제한 조회.
- 로컬 `destination` — `auto*` (추론 포인터); 목표 행성 위치.
- 로컬 `direction` — `const Vec3` (구조체); 목표 방향 벡터.

#### `executeAction(SetOrbitLevel)`

- 매개변수 `world` — `World&` (구조체 참조); 궤도 행성 검증.
- 매개변수 `ship` — `Ship&` (구조체 참조); 전이 대상.
- 매개변수 `action` — `const SetOrbitLevel&` (명령 구조체 참조); 목표 단계.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 함급 궤도 한계.

#### `executeAction(SetShipClass)`

- 매개변수 `ship` — `Ship&` (구조체 참조); 함급 변경 대상.
- 매개변수 `action` — `const SetShipClass&` (명령 구조체 참조); 새 함급.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 궤도 한계 조회.
- 로컬 `maxLevel` — `const auto` (추론 정수, `uint8_t`); 새 함급 최대 단계.

#### `executeAction(SelectTarget)`

- 매개변수 `world` — `World&` (구조체 참조); 엔티티 위치·속도 검색.
- 매개변수 `ship` — `Ship&` (구조체 참조); 대상 지정·조준 함선.
- 매개변수 `action` — `const SelectTarget&` (명령 구조체 참조); 선택 대상.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 무기 조준 규칙.
- 로컬 `targetPosition` — `auto*` (추론 포인터); 대상 위치.
- 로컬 `relativePosition` — `const Vec3` (구조체); 상대 위치.
- 로컬 `targetVelocity` — `Vec3` (구조체); 대상 속도.
- 로컬 `targetShip` — `const auto*` (추론 포인터); 대상 함선 검색 결과.
- 로컬 `targetTorpedo` — `const auto*` (추론 포인터); 대상 어뢰 검색 결과.
- 로컬 `weaponIndex` — `const auto` (추론 정수, `std::size_t`); 무기 인덱스.
- 로컬 `weapon` — `const WeaponProfile*` (구조체 포인터); 유효한 무기 프로필.
- 로컬 `aimDirection` — `const Vec3` (구조체); 예측 조준 방향.
- 로컬 `navigationActive` — `const bool` (불리언); 항법 제어 활성 여부.

### `src/engine/economy.cpp`

#### `executeAction(RepairVessel)`

- 매개변수 `world` — `World&` (구조체 참조); 플레이어 기지 검색.
- 매개변수 `player` — `Player&` (구조체 참조); 소유 기지 ID 조회.
- 매개변수 `ship` — `Ship&` (구조체 참조); 수리 대상.
- 매개변수 `action` — `const RepairVessel&` (명령 구조체 참조); 요청 수리량.
- 로컬 `base` — `auto*` (추론 포인터); 플레이어 기지.
- 로컬 `repaired` — `const double` (실수); 실제 수리량.

#### `executeAction(TransferResources)`

- 매개변수 `world` — `World&` (구조체 참조); 수신 대상 검색.
- 매개변수 `player` — `Player&` (구조체 참조); 이 처리에서는 미사용.
- 매개변수 `ship` — `Ship&` (구조체 참조); 자원 발신 함선.
- 매개변수 `action` — `const TransferResources&` (명령 구조체 참조); 이동 요청.
- 로컬 `targetShip` — `auto*` (추론 포인터); 대상 함선.
- 로컬 `targetBase` — `auto*` (추론 포인터); 대상 기지.
- 로컬 `targetPosition` — `const Vec3&` (구조체 참조); 대상 위치.
- 로컬 `destination` — `Resources&` (구조체 참조); 대상 자원.
- 로컬 `sourceValue` — `double&` (실수 참조); 발신 자원 필드.
- 로컬 `destinationValue` — `double&` (실수 참조); 수신 자원 필드.
- 로컬 `moved` — `const double` (실수); 실제 이동량.

#### `executeAction(LoadFromPlanet)`

- 매개변수 `world` — `World&` (구조체 참조); 행성 목록·위치 접근.
- 매개변수 `ship` — `Ship&` (구조체 참조); 적재 대상.
- 매개변수 `action` — `const LoadFromPlanet&` (명령 구조체 참조); 적재 요청.
- 로컬 `planet` — `auto&` (추론 참조, `Planet`); 적재 대상 행성.
- 로컬 `stock` — `double&` (실수 참조); 행성 재고.
- 로컬 `cargo` — `double&` (실수 참조); 함선 화물.
- 로컬 `loaded` — `const double` (실수); 실제 적재량.

#### `executeAction(SetAlliances)`

- 매개변수 `world` — `World&` (구조체 참조); 팀 동맹 목록 갱신.
- 매개변수 `action` — `const SetAlliances&` (명령 구조체 참조); 팀·동맹 변경 요청.
- 람다 매개변수 `team` — `Team&` (구조체 참조); 갱신할 팀.
- 람다 매개변수 `ally` — `TeamId` (`uint32_t`, 정수); 추가·제거할 팀 ID.
- 로컬 `found` — `auto` (추론 반복자); 동맹 목록 검색 결과.

### `src/engine/combat.cpp`

#### `applyTorpedoBlast`

- 매개변수 `world` — `World&` (구조체 참조); 폭발 피해 대상 월드.
- 매개변수 `torpedo` — `const Torpedo&` (구조체 참조); 폭발한 어뢰.
- 매개변수 `radius` — `double` (실수); 피해 반경.
- 반복 변수 `slot` — `std::optional<Ship>&` (선택적 구조체 참조); 피해 대상 슬롯.
- 로컬 `distance` — `const double` (실수); 폭발 중심까지 거리.
- 로컬 `damage` — `const double` (실수); 감쇠 적용 피해.
- 로컬 `shieldDamage` — `const double` (실수); 방어막 흡수량.

#### `dot`

- 매개변수 `lhs` — `const Vec3&` (구조체 참조); 첫 번째 벡터.
- 매개변수 `rhs` — `const Vec3&` (구조체 참조); 두 번째 벡터.

#### `applyProjectileImpact`

- 매개변수 `world` — `World&` (구조체 참조); 명중 대상·팀 검색 월드.
- 매개변수 `projectile` — `WeaponProjectile&` (구조체 참조); 명중 투사체.
- 로컬 `torpedo` — `auto*` (추론 포인터); 대상 어뢰.
- 로컬 `targetCombat` — `auto*` (추론 포인터); 대상 전투 상태.
- 로컬 `targetTeam` — `const auto` (추론 선택값, `optional<TeamId>`); 대상 팀.
- 로컬 `shieldDamage` — `const double` (실수); 방어막 흡수 피해.

#### `executeAction(FirePhasor)`

- 매개변수 `world` — `World&` (구조체 참조); 무기·대상·팀 검색.
- 매개변수 `player` — `Player&` (구조체 참조); 발사자·투사체 크기 설정.
- 매개변수 `ship` — `Ship&` (구조체 참조); 발사 함선.
- 매개변수 `action` — `const FirePhasor&` (명령 구조체 참조); 발사 출력.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 발사각·조준 계산.
- 로컬 `weaponIndex` — `const auto` (추론 정수, `std::size_t`); 무기 인덱스.
- 로컬 `weapon` — `const auto&` (추론 참조, `WeaponProfile`); 선택 프로필.
- 로컬 `cost` — `const double` (실수); 발사 비용.
- 로컬 `targetId` — `const EntityId` (`uint32_t`, 정수); 선택 대상 ID.
- 로컬 `targetTeam` — `const auto` (추론 선택값, `optional<TeamId>`); 대상 팀.
- 로컬 `targetPosition` — `auto*` (추론 포인터); 대상 위치.
- 로컬 `direction` — `const Vec3` (구조체); 대상 방향.
- 로컬 `distance` — `const double` (실수); 대상 거리.
- 로컬 `targetVelocity` — `Vec3` (구조체); 대상 속도.
- 로컬 `targetShip` — `const auto*` (추론 포인터); 대상 함선.
- 로컬 `targetTorpedo` — `const auto*` (추론 포인터); 대상 어뢰.
- 로컬 `hit` — `WeaponProjectile` (구조체); 즉발 명중용 임시 투사체.
- 로컬 `aim` — `const Vec3` (구조체); 예측 발사 방향.

#### `executeAction(FireTorpedo)`

- 매개변수 `world` — `World&` (구조체 참조); 어뢰 슬롯·초기 설정 조회.
- 매개변수 `player` — `Player&` (구조체 참조); 어뢰 소유자 정보.
- 매개변수 `ship` — `Ship&` (구조체 참조); 발사 함선.
- 매개변수 `action` — `const FireTorpedo&` (명령 구조체 참조); 발사 설정.
- 로컬 `torpedoSlot` — `auto*` (추론 포인터); 함선 어뢰 슬롯.
- 로컬 `aim` — `Vec3` (구조체); 사용할 발사 방향.
- 로컬 `targetPosition` — `const Vec3*` (구조체 포인터); 선택 대상 위치.
- 로컬 `aimLength` — `const double` (실수); 방향 벡터 길이.
- 로컬 `torpedo` — `Torpedo` (구조체); 새 어뢰 임시 객체.

#### `executeAction(DetonateTorpedo)`

- 매개변수 `world` — `World&` (구조체 참조); 폭발 처리 월드.
- 매개변수 `ship` — `Ship&` (구조체 참조); 활성 어뢰 ID를 제공.
- 로컬 `torpedo` — `auto*` (추론 포인터); 활성 어뢰.

#### `executeAction(SetWeaponProfile)`

- 매개변수 `world` — `World&` (구조체 참조); 무기 ID 범위 검사.
- 매개변수 `player` — `Player&` (구조체 참조); 선택 무기 갱신.
- 매개변수 `action` — `const SetWeaponProfile&` (명령 구조체 참조); 새 무기 ID.

#### `updateTorpedoes`

- 매개변수 `world` — `World&` (구조체 참조); 업데이트 대상.
- 반복 변수 `slot` — `std::optional<Torpedo>&` (선택적 구조체 참조); 처리 중인 어뢰 슬롯.
- 로컬 `dt` — `const double` (실수); 고정 시간 간격.
- 람다 매개변수 `planet` — `const Planet&` (구조체 참조); 충돌 검사 행성.
- 람다 로컬 `offset` — `const Vec3` (구조체); 어뢰-행성 상대 위치.
- 람다 로컬 `radius` — `const double` (실수); 충돌 반경 합.
- 로컬 `hitPlanet` — `const bool` (불리언); 행성 충돌 여부.
- 로컬 `impact` — `bool` (불리언); 함선 충돌 여부.
- 반복 변수 `targetSlot` — `const std::optional<Ship>&` (선택적 구조체 참조); 충돌 검사 함선 슬롯.
- 로컬 `offset` — `const Vec3` (구조체); 어뢰-함선 상대 위치.
- 로컬 `radius` — `const double` (실수); 충돌 반경 합.

#### `updateWeaponProjectiles`

- 매개변수 `world` — `World&` (구조체 참조); 투사체·대상 업데이트 월드.
- 로컬 `dt` — `const double` (실수); 고정 시간 간격.
- 람다 매개변수 `projectile` — `WeaponProjectile&` (구조체 참조); 갱신 중인 투사체.
- 로컬 `targetPosition` — `const Vec3*` (구조체 포인터); 목표 위치.
- 로컬 `targetVelocity` — `Vec3` (구조체); 목표 속도.
- 로컬 `targetRadius` — `double` (실수); 목표 충돌 반경.
- 로컬 `targetShip` — `const auto*` (추론 포인터); 목표 함선.
- 로컬 `targetBase` — `const auto*` (추론 포인터); 목표 기지.
- 로컬 `targetTorpedo` — `const auto*` (추론 포인터); 목표 어뢰.
- 로컬 `stepSeconds` — `const double` (실수); 이번 이동 시간.
- 로컬 `projectileStep` — `const Vec3` (구조체); 투사체 이동 벡터.
- 로컬 `relativeStart` — `const Vec3` (구조체); 시작 상대 위치.
- 로컬 `relativeStep` — `const Vec3` (구조체); 상대 이동량.
- 로컬 `relativeStepSquared` — `const double` (실수); 상대 이동량 제곱 길이.
- 로컬 `hitFraction` — `const double` (실수); 최근접 지점 비율.
- 로컬 `closestOffset` — `const Vec3` (구조체); 최근접 상대 위치.
- 로컬 `hitRadius` — `const double` (실수); 투사체·대상 반경 합.

### `src/engine/movement.cpp`

#### `dot` 및 `cross`

- 매개변수 `lhs` — `const Vec3&` (구조체 참조); 첫 번째 벡터.
- 매개변수 `rhs` — `const Vec3&` (구조체 참조); 두 번째 벡터.

#### `approachAngle`

- 매개변수 `current` — `double` (실수); 현재 각도.
- 매개변수 `target` — `double` (실수); 목표 각도.
- 매개변수 `maxDelta` — `double` (실수); 최대 회전량.
- 로컬 `delta` — `const double` (실수); 최단 각도 차.

#### `gravityAt`

- 매개변수 `world` — `const World&` (구조체 참조); 중력 행성 목록.
- 매개변수 `position` — `const Vec3&` (구조체 참조); 중력 계산 위치.
- 로컬 `acceleration` — `Vec3` (구조체); 누적 중력 가속도.
- 반복 변수 `planet` — `const Planet&` (구조체 참조); 중력 계산 행성.
- 로컬 `towardPlanet` — `const Vec3` (구조체); 행성 방향 벡터.
- 로컬 `distanceSquared` — `const double` (실수); 거리 제곱.
- 로컬 `radius` — `const double` (실수); 행성 최소 계산 반경.
- 로컬 `inverseDistance` — `const double` (실수); 역거리.
- 로컬 `scale` — `const double` (실수); 중력 크기 계수.

#### `resolvePlanetSurface`

- 매개변수 `world` — `const World&` (구조체 참조); 행성 목록.
- 매개변수 `ship` — `Ship&` (구조체 참조); 표면 충돌 보정 대상.
- 반복 변수 `planet` — `const Planet&` (구조체 참조); 충돌 검사 행성.
- 로컬 `fromCenter` — `const Vec3` (구조체); 중심-함선 벡터.
- 로컬 `collisionDistance` — `const double` (실수); 반경 합.
- 로컬 `distanceSquared` — `const double` (실수); 중심 거리 제곱.
- 로컬 `distance` — `const double` (실수); 중심 거리.
- 로컬 `normal` — `const Vec3` (구조체); 표면 바깥 방향.
- 로컬 `inwardSpeed` — `const double` (실수); 안쪽 속도 성분.

#### `integrateStableOrbit`

- 매개변수 `world` — `World&` (구조체 참조); 행성 목록 조회.
- 매개변수 `player` — `Player*` (구조체 포인터); 선택 대상 조향 정보.
- 매개변수 `ship` — `Ship&` (구조체 참조); 안정 궤도 함선.
- 매개변수 `dt` — `double` (실수); 고정 시간 간격.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 회전 조향 규칙.
- 로컬 `planet` — `const Planet&` (구조체 참조); 공전 대상 행성.
- 로컬 `orbitRadius` — `const double` (실수); 현재 궤도 반지름.
- 로컬 `relativePosition` — `const Vec3` (구조체); 행성 기준 함선 위치.
- 로컬 `radius` — `const double` (실수); 중심 거리.
- 로컬 `radial` — `const Vec3` (구조체); 방사 단위벡터.
- 로컬 `tangentVelocity` — `Vec3` (구조체); 속도의 접선 성분.
- 로컬 `tangent` — `const Vec3` (구조체); 접선 단위벡터.
- 로컬 `angularRate` — `const double` (실수); 궤도 각속도.
- 로컬 `angle` — `const double` (실수); 이번 틱 회전각.
- 로컬 `nextRadial` — `const Vec3` (구조체); 다음 방사 방향.
- 로컬 `nextTangent` — `const Vec3` (구조체); 다음 접선 방향.
- 로컬 `orbitSpeed` — `const double` (실수); 원궤도 속도.

#### `integrateOrbitTransfer`

- 매개변수 `world` — `World&` (구조체 참조); 행성 정보 조회.
- 매개변수 `ship` — `Ship&` (구조체 참조); 궤도 전이 함선.
- 매개변수 `dt` — `double` (실수); 고정 시간 간격.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 방사 전이 정책.
- 로컬 `planet` — `const Planet&` (구조체 참조); 공전 대상.
- 로컬 `targetLevel` — `const std::uint8_t` (정수); 목표 궤도 단계.
- 로컬 `targetRadius` — `const double` (실수); 목표 궤도 반지름.
- 로컬 `relativePosition` — `const Vec3` (구조체); 행성 기준 위치.
- 로컬 `radius` — `const double` (실수); 현재 행성 중심 거리.
- 로컬 `radial` — `const Vec3` (구조체); 방사 단위벡터.
- 로컬 `tangentVelocity` — `Vec3` (구조체); 속도의 접선 성분.
- 로컬 `tangent` — `const Vec3` (구조체); 접선 단위벡터.
- 로컬 `normal` — `Vec3` (구조체); 궤도면 법선.
- 로컬 `normalLength` — `const double` (실수); 법선 길이.
- 로컬 `requestedStep` — `const double` (실수); 요청 방사 이동량.
- 로컬 `radialStep` — `const double` (실수); 제한 적용 방사 이동량.
- 로컬 `nextRadius` — `const double` (실수); 다음 중심 거리.
- 로컬 `gravitationalParameter` — `const double` (실수); 행성 중력계수.
- 로컬 `orbitalRate` — `const double` (실수); 각속도.
- 로컬 `angle` — `const double` (실수); 이번 틱 회전각.
- 로컬 `nextRadial` — `const Vec3` (구조체); 다음 방사 방향.
- 로컬 `nextTangent` — `const Vec3` (구조체); 다음 접선 방향.
- 로컬 `targetSpeed` — `const double` (실수); 다음 원궤도 속도.
- 로컬 `radialSpeed` — `const double` (실수); 방사 속도.

#### `turnTowardGoal`

- 매개변수 `world` — `World&` (구조체 참조); 목표·대상 조회.
- 매개변수 `player` — `const Player*` (구조체 포인터); 선택 대상 ID.
- 매개변수 `ship` — `Ship&` (구조체 참조); 회전 대상.
- 매개변수 `dt` — `double` (실수); 회전 시간 간격.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 조준 방향 계산.
- 로컬 `direction` — `const Vec3` (구조체); 목표 행성 방향.
- 로컬 `target` — `const Target` (구조체); 선택 엔티티를 나타냄.
- 로컬 `relativePosition` — `const Vec3` (구조체); 대상 상대 위치.
- 로컬 `targetVelocity` — `Vec3` (구조체); 대상 속도.
- 로컬 `targetShip` — `const auto*` (추론 포인터); 대상 함선.
- 로컬 `targetTorpedo` — `const auto*` (추론 포인터); 대상 어뢰.
- 로컬 `weaponIndex` — `const auto` (추론 정수, `std::size_t`); 무기 인덱스.
- 로컬 `weapon` — `const WeaponProfile*` (구조체 포인터); 무기 프로필.
- 로컬 `aimDirection` — `const Vec3` (구조체); 예측 조준 방향.
- 로컬 `maxTurn` — `const double` (실수); 최대 회전량.

#### `updateApproachThrottle`

- 매개변수 `world` — `World&` (구조체 참조); 목표 행성 검색.
- 매개변수 `ship` — `Ship&` (구조체 참조); 접근 스로틀 갱신.
- 매개변수 `dt` — `double` (실수); 고정 시간 간격.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 속도·접근 정책.
- 로컬 `target` — `const Planet&` (구조체 참조); 목표 행성.
- 로컬 `distanceToInfluence` — `const double` (실수); 영향권까지 거리.
- 로컬 `maximumSpeed` — `const double` (실수); 최대 속도.
- 로컬 `towardTarget` — `const Vec3` (구조체); 목표 방향 벡터.
- 로컬 `distance` — `const double` (실수); 목표까지 거리.
- 로컬 `closingSpeed` — `const double` (실수); 접근 속도 성분.

#### `commandedThrust`

- 매개변수 `ship` — `const Ship&` (구조체 참조); 추력 계산 대상.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 가속·제동 설정.
- 매개변수 `maximumSpeed` — `double` (실수); 속도 상한.
- 로컬 `thrust` — `Vec3` (구조체); 추력 벡터.
- 로컬 `speed` — `const double` (실수); 현재 속력.
- 로컬 `velocityDirection` — `const Vec3` (구조체); 속도 단위벡터.
- 로컬 `forwardAcceleration` — `const double` (실수); 전방 추력 성분.

#### `captureGoalOrbit`

- 매개변수 `world` — `World&` (구조체 참조); 목표 행성 조회·상태 갱신.
- 매개변수 `player` — `Player*` (구조체 포인터); 플레이어 목표 갱신.
- 매개변수 `ship` — `Ship&` (구조체 참조); 포획 대상.
- 매개변수 `startPosition` — `const Vec3&` (구조체 참조); 틱 시작 위치.
- 로컬 `planet` — `const auto&` (추론 참조, `Planet`); 목표 행성.
- 로컬 `orbitRadius` — `const double` (실수); 영향권 반경.
- 로컬 `startOffset` — `const Vec3` (구조체); 시작 위치 상대 벡터.
- 로컬 `startDistanceSquared` — `const double` (실수); 시작 거리 제곱.
- 로컬 `influenceRadiusSquared` — `const double` (실수); 영향권 반경 제곱.
- 로컬 `segment` — `const Vec3` (구조체); 틱 이동 선분.
- 로컬 `segmentLengthSquared` — `const double` (실수); 선분 길이 제곱.
- 로컬 `closestFraction` — `const double` (실수); 최근접 지점 비율.
- 로컬 `closestOffset` — `const Vec3` (구조체); 최근접 지점 상대 위치.
- 로컬 `closestDistanceSquared` — `const double` (실수); 최근접 거리 제곱.
- 로컬 `halfChordFraction` — `const double` (실수); 교차 현 길이 비율.
- 로컬 `entryFraction` — `const double` (실수); 진입 지점 비율.
- 로컬 `requestedLevel` — `const auto` (추론 정수, `uint8_t`); 목표 궤도 단계.

#### `completeOrbitTransfer`

- 매개변수 `world` — `const World&` (구조체 참조); 행성 목록 조회.
- 매개변수 `ship` — `Ship&` (구조체 참조); 전이 완료 검사.
- 로컬 `planet` — `const auto&` (추론 참조, `Planet`); 전이 대상 행성.
- 로컬 `level` — `const auto` (추론 정수, `uint8_t`); 목표 궤도 단계.
- 로컬 `targetRadius` — `const double` (실수); 목표 반지름.
- 로컬 `relativePosition` — `const Vec3` (구조체); 행성 기준 위치.
- 로컬 `radius` — `const double` (실수); 현재 중심 거리.
- 로컬 `radial` — `const Vec3` (구조체); 방사 단위벡터.
- 로컬 `radialSpeed` — `const double` (실수); 방사 속도.
- 로컬 `tangent` — `Vec3` (구조체); 접선 성분.
- 로컬 `tangentialSpeed` — `const double` (실수); 접선 속력.
- 로컬 `targetSpeed` — `const double` (실수); 목표 원궤도 속도.

#### `integrateShip`

- 매개변수 `world` — `World&` (구조체 참조); 시뮬레이션 상태.
- 매개변수 `player` — `Player*` (구조체 포인터); 선택 대상 조향 정보.
- 매개변수 `ship` — `Ship&` (구조체 참조); 적분 대상.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 이동 규칙.
- 로컬 `dt` — `const double` (실수); 고정 시간 간격.
- 로컬 `previousPosition` — `const Vec3` (구조체); 스텝 시작 위치.
- 로컬 `speedLimit` — `double` (실수); 최대 속도 제한.
- 로컬 `target` — `const Planet&` (구조체 참조); 목표 행성.
- 로컬 `speedBeforeStep` — `const double` (실수); 가속 전 속력.
- 로컬 `speed` — `const double` (실수); 가속 후 속력.

#### 공개 이동 함수

- 매개변수 `world` — `const World&` (구조체 참조, gravityAcceleration); 또는 `World&` (구조체 참조, updateMovement).
- 매개변수 `position` — `const Vec3&` (구조체 참조); 중력 계산 위치.
- 매개변수 `rules` — `const GameRules&` (클래스 참조); 이동 규칙 전달.
- 반복 변수 `slot` — `std::optional<Ship>&` (선택적 구조체 참조); 처리 함선 슬롯.
- 로컬 `ship` — `auto&` (추론 참조, `Ship`); 유효한 함선.

### `src/engine/engine.cpp`

#### `selectedShipIds`

- 매개변수 `player` — `const Player&` (구조체 참조); 선택 함선 ID 목록 조회.

#### `isControlledShip`

- 매개변수 `ship` — `const Ship*` (구조체 포인터); 제어 가능 여부 검사.
- 매개변수 `playerId` — `PlayerId` (`uint32_t`, 정수); 소유권 비교.

#### `Simulation::Simulation`

- 매개변수 `world` — `World` (구조체 값); 시뮬레이션 초기 월드.
- 반복 변수 `planet` — `Planet&` (구조체 참조); 궤도 반경 설정 대상.
- 반복 변수 `level` — `std::uint8_t` (정수); 궤도 단계 인덱스.

#### `Simulation::execute`

- 매개변수 `playerId` — `PlayerId` (`uint32_t`, 정수); 명령 실행 플레이어.
- 매개변수 `command` — `const WorldCommand&` (variant 참조); 처리할 명령.
- 로컬 `player` — `auto*` (추론 포인터); 플레이어 검색 결과.
- 로컬 `ship` — `auto*` (추론 포인터); 대표 함선 검색 결과.
- 람다 매개변수 `action` — `const auto&` (추론 참조); variant의 실제 명령.
- 분기 로컬 `selection` — `const auto` (추론 컨테이너, `vector<EntityId>`); 선택 함선 목록.
- 반복 변수 `shipId` — `const EntityId` (`uint32_t`, 정수); 다중 선택 입력 ID.
- 반복 변수 `selectedId` — `const EntityId` (`uint32_t`, 정수); 현재 처리 함선 ID.
- 로컬 `selectedShip` — `auto*` 또는 `const auto*` (추론 포인터); 선택 함선 검색 결과.
- 로컬 `targetPosition` — `const auto*` (추론 포인터, `Vec3`); 선택 대상 위치.
- 로컬 `result` — `const auto` (추론 구조체, `CommandResult`); 개별 명령 결과.
- 로컬 `firedCount` — `std::size_t` (정수); 성공 발사 수.
- 로컬 `lastResult` — `CommandResult` (구조체); 마지막 발사·폭발 결과.
- 로컬 `completedCount` — `std::size_t` (정수); 성공 처리 어뢰 명령 수.
- 로컬 `actionName` — `const std::string` (문자열); 결과 메시지 접두어.

### `src/rules/scenario.cpp`

#### `makeShipDefaults`

- 매개변수 `turnRate` — `double` (실수); 기본 회전율.
- 매개변수 `skill` — `double` (실수); 기본 숙련도.
- 로컬 `ship` — `Ship` (구조체); 기본 함선 객체.

#### `makeTeam`

- 매개변수 `id` — `TeamId` (`uint32_t`, 정수); 팀 ID.
- 매개변수 `name` — `const char*` (문자열 포인터); 팀 이름.
- 매개변수 `baseOffset` — `const Vec3&` (구조체 참조); 기지 오프셋.
- 로컬 `team` — `Team` (구조체); 새 팀 객체.

#### `makePlanet`

- 매개변수 `id` — `PlanetId` (`uint8_t`, 정수); 행성 ID.
- 매개변수 `name` — `const char*` (문자열 포인터); 행성 이름.
- 매개변수 `position` — `Vec3` (구조체 값); 행성 위치.
- 매개변수 `radius` — `double` (실수); 행성 반지름.
- 매개변수 `gravity` — `double` (실수); 중력계수.
- 로컬 `planet` — `Planet` (구조체); 새 행성 객체.

#### `GameRules::GameRules`

- 반복 변수 `i` — `std::size_t` (정수); 팀·행성 검사 인덱스.
- 반복 변수 `team` — `const auto&` (추론 참조, `Team`); 검증 중인 팀.
- 반복 변수 `ally` — `const auto` (추론 정수, `TeamId`); 동맹 ID.
- 반복 변수 `practiceShip` — `const auto&` (추론 참조, `PracticeShipDefinition`); 연습 함선.
- 반복 변수 `weapon` — `const auto&` (추론 참조, `WeaponProfile`); 검증 중인 무기.
- 로컬 `aim` — `const auto&` (추론 참조, `WeaponAimSettings`); 무기 조준 설정.

#### `GameRules::applyScenario`

- 매개변수 `world` — `World&` (구조체 참조); 규칙 적용 대상.
- 로컬 `playerCapacity` — `const auto` (추론 정수, `std::size_t`); 플레이어 슬롯 수.
- 로컬 `shipCapacity` — `const auto` (추론 정수, `std::size_t`); 함선 슬롯 수.
- 로컬 `torpedoCapacity` — `const auto` (추론 정수, `std::size_t`); 어뢰 슬롯 수.
- 반복 변수 `i` — `std::size_t` (정수); 연습 함선 인덱스.
- 로컬 `definition` — `const auto&` (추론 참조, `PracticeShipDefinition`); 연습 함선 정의.
- 로컬 `target` — `Ship` (구조체); 새 연습 함선.

### `src/rules/calculations/flight.cpp`

#### `GameRules::speedLimit`

- 매개변수 `throttleLevel` — `std::uint8_t` (정수); 스로틀 단계.
- 매개변수 `gravitationalParameter` — `double` (실수); 행성 중력계수.
- 매개변수 `sphereOfInfluenceRadius` — `double` (실수); 영향권 반경.
- 로컬 `referenceSpeed` — `const double` (실수); 기준 궤도 속도.

#### `GameRules::approachThrottle`

- 매개변수 `distanceToCapture` — `double` (실수); 포획까지 거리.
- 매개변수 `currentSpeed` — `double` (실수); 현재 속력.
- 매개변수 `closingSpeed` — `double` (실수); 접근 속도.
- 매개변수 `maximumSpeed` — `double` (실수); 속도 상한.
- 매개변수 `deltaSeconds` — `double` (실수); 시간 간격.
- 로컬 `remaining` — `const double` (실수); 남은 제동 거리.
- 로컬 `safeSpeed` — `const double` (실수); 안전 접근 속도.
- 로컬 `targetSpeed` — `const double` (실수); 제한 적용 목표 속도.
- 로컬 `tolerance` — `const double` (실수); 판단 허용치.

### `src/rules/calculations/orbit.cpp`

#### `GameRules::maximumLevel`

- 매개변수 `shipClass` — `ShipClass` (enum); 최대 궤도 단계 조회 대상.
- 로컬 `index` — `const auto` (추론 정수, `std::size_t`); 함급 배열 인덱스.

#### `GameRules::orbitRadius`

- 매개변수 `planet` — `const Planet&` (구조체 참조); 반지름 계산 행성.
- 매개변수 `level` — `std::uint8_t` (정수); 궤도 단계.
- 로컬 `innerRadius` — `const double` (실수); 안전한 최내측 반지름.

#### `GameRules::transferRadialStep`

- 매개변수 `currentRadius` — `double` (실수); 현재 궤도 반지름.
- 매개변수 `targetRadius` — `double` (실수); 목표 궤도 반지름.
- 매개변수 `deltaSeconds` — `double` (실수); 전이 시간 간격.
- 로컬 `difference` — `const double` (실수); 반지름 차이.
- 로컬 `remaining` — `const double` (실수); 남은 전이 거리.
- 로컬 `speed` — `const double` (실수); 방사 속도.

### `src/rules/calculations/weapon_aim.cpp`

#### `dot`

- 매개변수 `lhs` — `const Vec3&` (구조체 참조); 첫 번째 벡터.
- 매개변수 `rhs` — `const Vec3&` (구조체 참조); 두 번째 벡터.

#### `GameRules::weaponAimDirection`

- 매개변수 `relativePosition` — `const Vec3&` (구조체 참조); 대상 상대 위치.
- 매개변수 `relativeVelocity` — `const Vec3&` (구조체 참조); 대상 상대 속도.
- 매개변수 `projectileSpeed` — `double` (실수); 투사체 속도.
- 매개변수 `aim` — `const WeaponAimSettings&` (구조체 참조); 선행 조준 설정.
- 로컬 `distanceSquared` — `const double` (실수); 거리 제곱.
- 로컬 `length` — `const double` (실수); 방향 벡터 길이.
- 로컬 `velocitySquared` — `const double` (실수); 상대 속도 제곱.
- 로컬 `a` — `const double` (실수); 이차항 계수.
- 로컬 `b` — `const double` (실수); 일차항 계수.
- 로컬 `time` — `double` (실수); 예측 요격 시간.
- 로컬 `discriminant` — `const double` (실수); 판별식.
- 로컬 `root` — `const double` (실수); 판별식 제곱근.
- 로컬 `firstTime` — `const double` (실수); 첫 요격 시간 후보.
- 로컬 `secondTime` — `const double` (실수); 둘째 요격 시간 후보.
- 로컬 `direction` — `const Vec3` (구조체); 예측 조준 방향.

#### `GameRules::isWithinFiringArc`

- 매개변수 `orientation` — `const Orientation&` (구조체 참조); 발사 함선 방향.
- 매개변수 `targetDirection` — `const Vec3&` (구조체 참조); 목표 방향 벡터.
- 매개변수 `aim` — `const WeaponAimSettings&` (구조체 참조); 발사각 설정.
- 로컬 `horizontalLength` — `const double` (실수); 수평 길이.
- 로컬 `toDegrees` — `constexpr double` (실수 상수); 라디안→도 변환.
- 로컬 `targetYaw` — `const double` (실수); 목표 요 각도.
- 로컬 `targetPitch` — `const double` (실수); 목표 피치 각도.
- 로컬 `horizontalError` — `const double` (실수); 수평 조준 오차.
- 로컬 `verticalError` — `const double` (실수); 수직 조준 오차.

## 클래스 연결 요약

```text
Simulation
├── world_ : World       # 엔티티·시간 상태; step/execute에서 엔진 구현부로 전달
└── gameRules_ : GameRules # 시나리오 규칙; 이동·항법·무기 계산에 전달

World
├── Entity 멤버 구조체들 # ships/bases/torpedoes 등에 저장
├── 시간·ID·난수 상태   # 월드 생성과 매 틱 처리에서 사용
└── 기본값·설정         # GameRules::applyScenario가 초기화

WorldCommand
└── 명령 구조체 데이터  # Simulation::execute가 해당 executeAction으로 전달

구현 함수
├── 매개변수            # 호출 경계에서 전달되는 입력·상태 참조
└── 로컬 변수           # 해당 함수나 람다에서만 쓰는 임시 계산값
```
