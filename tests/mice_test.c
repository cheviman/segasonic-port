/* Who is whose in ports/sdl2/mice.c: build with any C compiler, `cc -DMICE_TEST -I ports/sdl2 tests/mice_test.c
   ports/sdl2/mice.c -o mice_test`, and run it: it prints "mice: ok" or the first thing that is wrong */
#include "mice.h"

#include <stdio.h>
#include <stdlib.h>

static void check(bool condition, const char *what)
{
	if (!condition)
	{
		printf("mice: FAILED: %s\n", what);
		exit(1);
	}
}

static void take(int player, int *dx, int *dy, bool *down, bool *has)
{
	unsigned buttons;

	*has = mice_take(player, dx, dy, &buttons);
	*down = buttons != 0;
}

int main(void)
{
	int dx, dy;
	bool down, has;

	check(!mice_seen(), "nothing seen at first");
	mice_event(0, 5, 5, false, 0);
	check(!mice_seen(), "a device of 0 is nothing");

	/* the first mouse that moves is player 1's, with no button */
	mice_event(101, 3, -2, false, 0);
	check(mice_seen(), "a mouse is seen");
	take(0, &dx, &dy, &down, &has);
	check(has && dx == 3 && dy == -2 && !down, "the first mouse moves player 1's ball");
	take(0, &dx, &dy, &down, &has);
	check(dx == 0 && dy == 0, "what was taken is gone");

	/* a second mouse that only moves is nobody's; its first button makes it player 2's */
	mice_event(202, 9, 9, false, 0);
	take(1, &dx, &dy, &down, &has);
	check(!has && dx == 0, "a mouse that only moves takes no player");
	mice_event(202, 0, 0, true, 1u << 2);
	take(1, &dx, &dy, &down, &has);
	check(has && down, "a button gives player 2 its mouse");
	mice_event(202, 4, 1, false, 0);
	take(1, &dx, &dy, &down, &has);
	check(dx == 4 && dy == 1 && down, "player 2's ball follows its own mouse");
	take(0, &dx, &dy, &down, &has);
	check(dx == 0 && dy == 0 && !down, "and not player 1's");

	/* the buttons let go */
	mice_event(202, 0, 0, true, 0);
	take(1, &dx, &dy, &down, &has);
	check(!down, "the button is let go");

	/* a third, then a fourth that finds no player free */
	mice_event(303, 0, 0, true, 1);
	take(2, &dx, &dy, &down, &has);
	check(has && down, "a third mouse is player 3's");
	mice_event(404, 7, 7, true, 1);
	for (int player = 0; player < MICE_PLAYERS; player++)
	{
		take(player, &dx, &dy, &down, &has);
		check(dx == 0 && dy == 0, "a fourth mouse moves nobody's ball");
	}

	/* a mouse unplugged frees its player for the next one to press a button */
	mice_remove(202);
	take(1, &dx, &dy, &down, &has);
	check(!has, "an unplugged mouse leaves its player");
	mice_event(404, 0, 0, true, 0);
	mice_event(404, 0, 0, true, 1);
	take(1, &dx, &dy, &down, &has);
	check(has && down, "the next mouse to press takes the freed player");

	mice_reset();
	take(0, &dx, &dy, &down, &has);
	check(!has, "reset forgets every mouse");

	printf("mice: ok\n");
	return 0;
}
