#include "../include/timer.h"

//Probs should declare volatile
static timer::Runtime runtime;

#ifndef __INTELLISENSE__
ISR(TIMER0_OVF_vect) {
	if (runtime.loop_index == runtime.param.loop) {
		if (runtime.loop_remainder) {
			timer::tick();
			timer::next_tick();
		}
		else {
			runtime.loop_remainder = true;
			TCNT0 = 0xff - runtime.param.home;
		}
	}
	else {
		runtime.loop_index++;
	}
}
#endif

void timer::next_tick() {
#ifndef __INTELLISENSE__
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#endif
		//If it needs to do a loop
		if (runtime.param.loop != 0) {
			runtime.loop_index = 0;
			runtime.loop_remainder = false;
		}
		//And if it's going straight to the remainder
		else {
			runtime.loop_remainder = true;
			TCNT0 = 0xff - runtime.param.home;
		}
#ifndef __INTELLISENSE__
	}
#endif
}

void timer::init(double const interval) {
#ifndef __INTELLISENSE__
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#endif
		runtime.param = determine_parameters(interval);
		next_tick();
		TCCR0 = runtime.param.prescale;
		TIMSK |= _BV(TOIE0);
#ifndef __INTELLISENSE__
	}
#endif
}

void timer::add(Timer *const ntimer) {
#ifndef __INTELLISENSE__
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#endif
		runtime.timer_amount++;
		runtime.timer_buf = static_cast<Timer **>(realloc(static_cast<void *>(runtime.timer_buf), sizeof(Timer *) * runtime.timer_amount));
		runtime.timer_buf[runtime.timer_amount - 1] = ntimer;
#ifndef __INTELLISENSE__
	}
#endif
}

void timer::remove(Timer const *const ntimer) {
#ifndef __INTELLISENSE__
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#endif
		size_t pos = find(ntimer);
		memcpy(runtime.timer_buf + pos, runtime.timer_buf + pos + 1, runtime.timer_amount - pos - 1);
		runtime.timer_amount--;
		runtime.timer_buf = static_cast<Timer **>(realloc(static_cast<void *>(runtime.timer_buf), sizeof(Timer *) * runtime.timer_amount));
#ifndef __INTELLISENSE__
	}
#endif
}

size_t timer::find(Timer const *const ntimer) {
#ifndef __INTELLISENSE__
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#endif
		for (size_t i = 0; i < runtime.timer_amount; i++) {
			if (ntimer == runtime.timer_buf[i])
				return(i);
		}
#ifndef __INTELLISENSE__
	}
#endif
	return(0);
}

void timer::tick() {
#ifndef __INTELLISENSE__
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#endif
		for (size_t i = 0; i < runtime.timer_amount; i++) {
			runtime.timer_buf[i]->decrement();
		}
#ifndef __INTELLISENSE__
	}
#endif
}

Timer::operator bool() const {
	return(finished);
}

Timer::operator uint64_t() const {
	return(ticks);
}

void Timer::decrement() {
	if (running) {
		ticks--;
		if (ticks == 0) {
			running = false;
			finished = true;
		}
	}
}

Timer &Timer::operator=(uint64_t const p0) {
	ticks = p0;
	return(*this);
}

void Timer::start() {
	running = true;
	finished = false;
}

void Timer::stop() {
	running = false;
}

void Timer::reset() {
	finished = false;
	running = false;
	ticks = 0;
}

Timer::Timer() {
	timer::add(this);
}

Timer::~Timer() {
	timer::remove(this);
}
