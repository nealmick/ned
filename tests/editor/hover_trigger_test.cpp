#include "views/hover_trigger.h"

#include "third_party/catch.hpp"

namespace {

struct Clock
{
	double now = 0.0;
	double operator()() const { return now; }
};

} // namespace

TEST_CASE("HoverTrigger shows after the mouse rests, hides on any movement",
		  "[ned][hover]")
{
	Clock clock;
	HoverTrigger trigger([&clock] { return clock.now; });
	trigger.setDelayForTest(0.3);

	HoverTrigger::Target word{HoverTrigger::Zone::Text, 4, 10};

	// A move arms; the delay elapses on stationary frames after the move.
	trigger.update(true, false, word);
	clock.now += 0.1;
	trigger.update(false, false, word);
	REQUIRE_FALSE(trigger.info().active);
	clock.now += 0.3;
	trigger.update(false, false, word);
	REQUIRE(trigger.info().active);
	REQUIRE(trigger.info().row == 4);
	REQUIRE(trigger.info().column == 10);

	// ANY movement hides immediately — even staying on the same target.
	trigger.update(true, false, word);
	REQUIRE_FALSE(trigger.info().active);

	// Resting again re-shows after a fresh delay (re-armed by the move).
	clock.now += 0.1;
	trigger.update(false, false, word);
	REQUIRE_FALSE(trigger.info().active);
	clock.now += 0.3;
	trigger.update(false, false, word);
	REQUIRE(trigger.info().active);
}

TEST_CASE("HoverTrigger dismissal by key/click/scroll hides immediately", "[ned][hover]")
{
	Clock clock;
	HoverTrigger trigger([&clock] { return clock.now; });
	trigger.setDelayForTest(0.05);

	HoverTrigger::Target word{HoverTrigger::Zone::Text, 0, 5};
	trigger.update(true, false, word);
	clock.now += 0.1;
	trigger.update(false, false, word);
	REQUIRE(trigger.info().active);

	SECTION("dismissed while resting") { trigger.update(false, true, word); }
	SECTION("dismissed with movement") { trigger.update(true, true, word); }
	SECTION("left every zone") { trigger.update(false, false, HoverTrigger::Target{}); }

	REQUIRE_FALSE(trigger.info().active);
}

TEST_CASE("HoverTrigger stays hidden after keydown until the mouse moves (VSCode rule)",
		  "[ned][hover]")
{
	Clock clock;
	HoverTrigger trigger([&clock] { return clock.now; });
	trigger.setDelayForTest(0.05);

	HoverTrigger::Target word{HoverTrigger::Zone::Text, 0, 5};

	// Arm via movement, let it fire.
	trigger.update(true, false, word);
	clock.now += 0.1;
	trigger.update(false, false, word);
	REQUIRE(trigger.info().active);

	// Arrow key: hide.
	trigger.update(false, true, word);
	REQUIRE_FALSE(trigger.info().active);

	// Parked mouse must NOT bring it back, no matter how long we wait.
	clock.now += 10.0;
	trigger.update(false, false, word);
	REQUIRE_FALSE(trigger.info().active);
	clock.now += 10.0;
	trigger.update(false, false, word);
	REQUIRE_FALSE(trigger.info().active);

	// Only an actual mouse move re-arms.
	trigger.update(true, false, word);
	REQUIRE_FALSE(trigger.info().active);
	clock.now += 0.1;
	trigger.update(false, false, word);
	REQUIRE(trigger.info().active);
}

TEST_CASE("HoverTrigger never fires while the mouse keeps moving", "[ned][hover]")
{
	Clock clock;
	HoverTrigger trigger([&clock] { return clock.now; });
	trigger.setDelayForTest(0.05);

	HoverTrigger::Target word{HoverTrigger::Zone::Text, 1, 3};
	for (int i = 0; i < 10; ++i)
	{
		trigger.update(true, false, word);
		clock.now += 0.1;
	}
	REQUIRE_FALSE(trigger.info().active);
}
