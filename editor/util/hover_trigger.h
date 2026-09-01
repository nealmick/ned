#pragma once

/*
	Transient hover trigger, one machine for all hover consumers (symbol
	hover, squiggle tooltip, gutter tooltip). Rules mirror VSCode's
	contentHoverController / hoverOperation:

	1. The delay timer is started ONLY by a mouse-move event
	   (hoverOperation.start is called solely from the mouse-move path).
	2. Keystroke, click/drag, blocked input, scroll, or leaving every hover
	   zone hides the hover AND cancels the timer. Nothing re-arms until the
	   mouse actually moves again — keydown-hide then resting mouse stays
	   hidden (VSCode _onKeyDown only calls hideContentHover).
	3. A mouse move while showing retargets: hide, then re-arm the delay at
	   the new position.
	4. Content shifting under a parked mouse never (re)triggers: arming
	   requires a movement, and stationary frames only let the already-armed
	   timer elapse.

	Popup stickiness (reading a visible popup) is handled by the popup owner
	via its own rectangle; it cannot mutate this machine.
*/

#include "imgui.h"
#include <algorithm>
#include <functional>

class HoverTrigger
{
  public:
	enum class Zone {
		None,
		Text,	// document pane: row + UTF-8 byte column
		Gutter, // line-number column: row only
	};

	struct Info
	{
		bool active = false;
		Zone zone = Zone::None;
		int row = -1;
		int column = 0; // UTF-8 byte offset into the row (Text zone)
	};

	struct Target
	{
		Zone zone = Zone::None;
		int row = -1;
		int column = 0;
	};

	// Clock injectable for tests; production uses ImGui::GetTime().
	explicit HoverTrigger(std::function<double()> clock = &ImGui::GetTime)
		: clock(std::move(clock))
	{
	}

	// Feed once per frame, after layout/input/scroll.
	void update(bool mouseMoved, bool dismissed, const Target &target)
	{
		if (mouseMoved || dismissed || target.zone == Zone::None)
		{
			armed = false;
			current = Info{};
		}

		if (mouseMoved && !dismissed && target.zone != Zone::None)
		{
			// Rule 1: only a mouse move arms the delay.
			armed = true;
			armedAt = clock();
			armedTarget = target;
		} else if (armed && !current.active && target.zone != Zone::None &&
				   clock() - armedAt >= delaySeconds())
		{
			current.active = true;
			current.zone = armedTarget.zone;
			current.row = armedTarget.row;
			current.column = armedTarget.column;
		}
	}

	const Info &info() const { return current; }

	// Tests only: deterministic delay (default -1 = ImGui hover delay, min 0.25s).
	void setDelayForTest(double seconds) { testDelay = seconds; }

  private:
	double delaySeconds() const
	{
		return testDelay >= 0.0
				   ? testDelay
				   : std::max(kMinHoverDelaySeconds,
							  static_cast<double>(ImGui::GetStyle().HoverDelayNormal));
	}

	// Floor below the ImGui style delay: a hover that flickers faster than
	// this is unreadable.
	static constexpr double kMinHoverDelaySeconds = 0.25;

	std::function<double()> clock;
	double testDelay = -1.0;

	bool armed = false;
	double armedAt = 0.0;
	Target armedTarget;
	Info current;
};
