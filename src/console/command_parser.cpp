#include "console/command_parser.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <algorithm>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <system_error>

namespace spasim
{
namespace
{
std::string lower(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

bool parseEntityIdToken(std::string token, std::uint32_t &id)
{
  token = lower(std::move(token)); // 입력을 소문자화

  if (token.starts_with("ship-")) // ship-로 시작하면 앞의 5글자를 제거. 예: ship-42 → 42
    token.erase(0, 5);
  else if (token.starts_with('t') && token.size() > 1) // t로 시작하고 뒤에 글자가 더 있으면 맨 앞의 t를 제거. 예: t42 → 42
    token.erase(0, 1);

  if (token.starts_with('#')) // 남은 문자열이 #로 시작하면 그 문자도 제거. 예: ship-#42 → 42
    token.erase(0, 1);
  if (token.empty()) // 접두사를 제거한 뒤 아무것도 남지 않았다면 ID로 읽을 수 없으므로 실패를 반환.
    return false;

  const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), id);
  // token.data(): 문자열의 첫 문자 위치
  // token.data() + token.size(): 문자열 끝 위치
  // id: 숫자로 변환한 결과를 저장할 변수
  /*
   * 예를 들어 token이 "123abc"라면 123까지만 숫자로 읽고, end는 a를 가리킵니다. 그래서 뒤의 검사에서 end가 문자열 끝과 같은지 확인해
   * "123abc" 전체가 숫자가 아니라는 걸 판단합니다. 반대로 "123"이면 오류가 없고 end가 문자열 끝에 도달해 변환이 성공합니다.
   */

  return error == std::errc{} && end == token.data() + token.size();
  // error == std::errc{}: 변환 중 오류가 없었는지 확인합니다.
  // end == token.data() + token.size(): 숫자를 읽은 위치가 문자열 끝인지 확인합니다.
}
} // namespace

Command parseCommand(std::string_view line)
{
  /*
   * std::string처럼 문자를 직접 소유하는 객체가 아니라, 이미 있는 문자열의 일부를 읽기 전용으로 가리키는 가벼운 참조.
   * 다만 string_view는 원본 문자열을 소유하지 않으므로, 원본이 사라지면 view도 유효하지 않게 됩니다. 이 함수에서는 뒤에서
   * std::string(line)으로 복사본을 만들기 때문에, 그 복사본을 istringstream으로 읽는 구조입니다.
   */
  std::istringstream input{std::string(line)};
  // std::string(line)은, line의 내용을 std::string으로 만듭니다.
  // std::istringstream은, 그 문자열을 입력 스트림처럼 읽게 해 줍니다.

  std::string commandName;
  input >> commandName;
  commandName = lower(std::move(commandName));

  // 명령어를 종류별로 구분합니다.
  Command result;
  if (commandName == "help" || commandName == "?")
    result.kind = CommandKind::Help;
  else if (commandName == "status" || commandName == "s")
    result.kind = CommandKind::Status;
  else if (commandName == "teams")
    result.kind = CommandKind::Teams;
  else if (commandName == "planets")
    result.kind = CommandKind::Planets;
  else if (commandName == "tick" || commandName == "wait")
    result.kind = CommandKind::Tick;
  else if (commandName == "turn")
    result.kind = CommandKind::Turn;
  else if (commandName == "accel")
    result.kind = CommandKind::Accelerate;
  else if (commandName == "decel")
    result.kind = CommandKind::Brake;
  else if (commandName == "coast")
    result.kind = CommandKind::Cruise;
  else if (commandName == "throttle")
    result.kind = CommandKind::SetThrottleLevel;
  else if (commandName == "goal")
    result.kind = CommandKind::SetGoal;
  else if (commandName == "orbit")
    result.kind = CommandKind::SetOrbitLevel;
  else if (commandName == "class")
    result.kind = CommandKind::SetShipClass;
  else if (commandName == "target")
    result.kind = CommandKind::Select;
  else if (commandName == "shoot")
    result.kind = CommandKind::Fire;
  else if (commandName == "launch")
    result.kind = CommandKind::FireTorpedo;
  else if (commandName == "detonate")
    result.kind = CommandKind::Detonate;
  else if (commandName == "weapon")
    result.kind = CommandKind::SetWeapon;
  else if (commandName == "repair")
    result.kind = CommandKind::Repair;
  else if (commandName == "transfer")
    result.kind = CommandKind::Transfer;
  else if (commandName == "load")
    result.kind = CommandKind::Load;
  else if (commandName == "ally")
    result.kind = CommandKind::Ally;
  else if (commandName == "pause")
    result.kind = CommandKind::Pause;
  else if (commandName == "resume")
    result.kind = CommandKind::Resume;
  else if (commandName == "speed")
    result.kind = CommandKind::Speed;
  else if (commandName == "snapshot")
    result.kind = CommandKind::Snapshot;
  else if (commandName == "exit")
    result.kind = CommandKind::Quit;

  std::string first;
  std::string second;
  std::string third;
  std::string fourth;
  input >> first;
  input >> second;
  input >> third;
  input >> fourth;

  result.text = first;
  /* NOTE:
   * 첫 번째 인자를 result의 text 멤버에 저장합니다. 그다음 코드에서 CommandKind에 따라 first, second 등의 값을 숫자로 변환하거나,
   * 엔티티 ID로 해석해요. 즉, 이 부분은 우선 인자를 문자열로 받아 두는 단계입니다.
   */

  auto tryParseDouble = [](const std::string &text, double &value) {
    if (text.empty())
      return false;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    return error == std::errc{} && end == text.data() + text.size();
  };

  // 대체로 인자를 숫자로 제대로 파싱했는지를 나타냅니다.
  // 일부 선택 인자(SetGoal의 hasSecond)는 입력이 있었는지를 표시합니다.
  // OPTIMIZE:
  // bool 대신 비트 마스크 쓸수 있을듯. 선언을 구조체(struct)로 해서 의도대로 메모리가 안줄을수도 있음.
  if (result.kind == CommandKind::Turn)
  {
    result.hasFirst = tryParseDouble(first, result.first);
    result.hasSecond = tryParseDouble(second, result.second);
    result.hasThird = tryParseDouble(third, result.thirdValue);
  }
  else if (result.kind == CommandKind::Ally)
  {
    result.hasFirst = tryParseDouble(first, result.first);
    result.hasSecond = tryParseDouble(second, result.second);
  }
  else if (result.kind == CommandKind::SetGoal)
  {
    double target{};
    result.hasTarget = tryParseDouble(first, target) &&
                        target >= 1.0 &&
                        target <= 256.0 &&
                        std::floor(target) == target;

    if (result.hasTarget == true)
      result.target = static_cast<std::uint32_t>(target - 1.0);
    // result.hasTarget이 유효하면 입력값에서 1을 빼서 저장하는데,
    // 사용자 입력은 1부터 시작하고 내부 인덱스는 0부터 시작하기 때문이에요. 유효하지 않으면 저장하지 않음.
    // TODO: 그냥 사용자 입력토 타켓 0부터 시작하는것도 이후에 고려해볼만함.
    // 사용자가 고르는 숫자랑 실제프로그램이 처리하는 숫자랑 다른건 상당히 골치아픔.

    // 이부분은 두번째 인자를 숫자로 변환합니다.
    if (!second.empty())
    {
      result.hasSecond = true;
      tryParseDouble(second, result.second);
    }
  }
  else if (result.kind == CommandKind::SetOrbitLevel)
  {
    double level{};
    result.hasTarget = tryParseDouble(first, level) &&
                        level >= 1.0 &&
                        level <= kMaxOrbitLevels &&
                        std::floor(level) == level;

    if (result.hasTarget)
      result.target = static_cast<std::uint32_t>(level);
  }
  else if (result.kind == CommandKind::Fire)
  {
    result.hasFirst = tryParseDouble(first, result.first);
    if (!result.hasFirst && first.empty())
    {
      result.first = 1.0;
      result.hasFirst = true;
    }
  }
  else if (result.kind == CommandKind::FireTorpedo || result.kind == CommandKind::Repair)
  {
    result.hasFirst = tryParseDouble(first, result.first);
  }
  else if (result.kind == CommandKind::Select)
  {
    for (const auto &token : {first, second, third, fourth})
    {
      if (token.empty() || lower(token) == "ship")
        continue;
      if (parseEntityIdToken(token, result.target))
      {
        result.hasTarget = true;
        break;
      }
    }
  }
  else if (result.kind == CommandKind::SetWeapon)
  {
    double weapon{};
    result.hasTarget = tryParseDouble(first, weapon) && weapon >= 1.0 &&
                       weapon <= static_cast<double>(std::numeric_limits<WeaponId>::max()) + 1.0 &&
                       std::floor(weapon) == weapon;

    if (result.hasTarget)
      result.target = static_cast<std::uint32_t>(weapon - 1.0);
  }
  else if (result.kind == CommandKind::Transfer || result.kind == CommandKind::Load)
  {
    result.text = first;
    result.hasFirst = tryParseDouble(second, result.first);
    try
    {
      result.target = static_cast<std::uint32_t>(std::stoul(third));
      result.hasTarget = true;
    }
    catch (...)
    {
      result.hasTarget = false;
    }
  }
  else if (result.kind == CommandKind::Snapshot)
  {
    result.text = first;
  }
  else if (result.kind == CommandKind::Speed)
  {
    result.hasFirst = tryParseDouble(first, result.first);
  }
  else if (result.kind == CommandKind::SetThrottleLevel)
  {
    double level{};
    result.hasTarget = tryParseDouble(first, level) &&
                        level >= 1.0 &&
                        level <= 4.0 &&
                        std::floor(level) == level;

    if (result.hasTarget)
      result.target = static_cast<std::uint32_t>(level);
  }
  return result;
}

} // namespace spasim
