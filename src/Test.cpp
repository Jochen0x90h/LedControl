#include <Test.hpp> // drivers
#include "EffectManager.hpp"
#include "effects/StaticColor.hpp"
#include "effects/ColorFade.hpp"
#include "effects/ColorFade3.hpp"
#include "effects/ColorCycle3.hpp"
#include "effects/Strobe.hpp"
#include "effects/CylonBounce.hpp"
#include "effects/MeteorRain.hpp"
#include "effects/Spring.hpp"
#include "effects/Summer.hpp"
#include "effects/Autumn.hpp"
#include "effects/Winter.hpp"
#include <coco/Menu.hpp>
#include <coco/font/tahoma8pt1bpp.hpp>
#include <coco/PseudoRandom.hpp>
#include <coco/StreamOperators.hpp>
#include <coco/noise.hpp>
#include <coco/debug.hpp>


/*
	Test a single effect
*/

using namespace coco;




int main() {
	Strip strip(drivers.buffer1, drivers.buffer2);

	/*Winter::Parameters parameters;
	Winter::init(&parameters);
	parameters.size = 40;
	parameters.speed = 40;
	Winter::run(drivers.loop, strip, &parameters);*/

	StaticColor::Parameters parameters;
	StaticColor::init(&parameters);
	//parameters.hue = 0;
	StaticColor::run(drivers.loop, strip, &parameters);

	drivers.loop.run();
	return 0;
}
