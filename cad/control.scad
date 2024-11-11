// export checkist
// - resolution of circles high enough? ($fn = 128)
// - drill positions up to date? (gerber/drill2cad.py)
$fn = 128;

// variables
M2Diameter = 3.2; // recommended diameter for M2 insert
M2Wall = 1.3; // wall thickness for M2 insert
M2Depth = 5;

// box
boxWidth = 80;
boxHeight = 80;
boxX1 = -boxWidth/2;
boxX2 = boxWidth/2;
boxY1 = -boxHeight/2;
boxY2 = boxHeight/2;

// pcb
pcbX1 = -31;
pcbX2 = 31;
pcbY1 = -38;
pcbY2 = 3;
pcbX = (pcbX1 + pcbX2)/2;
pcbY = (pcbY1 + pcbY2)/2;
pcbWidth = pcbX2 - pcbX1;
pcbHeight = pcbY2 - pcbY1;
pcbZ1 = 3;
pcbZ2 = pcbZ1+1.6; // mounting surface

// pcb screw
pcbScrewX = -28;
pcbScrewY = -22;

// poti (Bourns PEC12R-4215F-S0024)
potiX = 22;
potiY = -22;
potiH1 = 7; // height of body above pcb (variant with switch)
potiH2 = 11.2; // height of shaft bearing above pcb
potiL = 17.5; // height of shaft end above pcb
potiF = 5; // shaft cutaway
potiZ1 = pcbZ2 + potiH1;
potiZ2 = pcbZ2 + potiH2;
potiZ4 = pcbZ2 + potiL;
potiZ3 = potiZ4 - potiF;
potiD1 = 7; // shaft bearing diameter
potiD2 = 6; // shaft diameter

// base
baseZ1 = 0;
baseZ2 = pcbZ1;

// cover
coverZ1 = pcbZ2;
coverZ2 = potiZ4 + 1; // use height of poti mounted on pcb plus cap
coverFit = 0.4;
coverOverlap = 2.5;

// wheel
wheelD1 = 10; // diameter of shaft
wheelD2 = 39; // diameter of wheel
wheelGap = 0.5; // visible air gab between box and wheel
wheelZ1 = potiZ3;
wheelZ2 = coverZ2 - 3; // thickness of wheel
wheelZ3 = coverZ2;

// photo diode
photoX = -potiX;
photoY = potiY;
photoD = 5.1 + 0.1; // diameter + tolerance
photoZ2 = coverZ2;
photoZ1 = photoZ2 - 3.85;

// display (2.42 inch)
displayTolerance = 0.4;
displayCableWidth1 = 13;
displayCableWidth2 = 16; // where the flat cable is connected to display
displayCableLength = 35;

// display screen (active area of display)
screenX = 0;
screenY = 20.5;
screenWidth = 57.01;
screenHeight = 29.49;
screenOffset = 1.08 + displayTolerance; // distance between upper panel border and upper screen border (tolerance at upper border)
screenX1 = screenX - screenWidth/2;
screenX2 = screenX + screenWidth/2;
screenY1 = screenY - screenHeight/2;
screenY2 = screenY + screenHeight/2;

// display panel (glass carrier)
panelWidth = 60.5 + displayTolerance;
panelHeight = 37 + displayTolerance;
panelThickness1 = 1.2; // at side where cable is connected
panelThickness2 = 2.3;
panelX = screenX;
panelX1 = panelX - panelWidth / 2; // left border of panel
panelX2 = panelX + panelWidth / 2; // right border of panel
panelY2 = screenY + screenHeight/2+screenOffset; // upper border of panel
panelY1 = panelY2-panelHeight; // lower border of panel
panelY = (panelY2+panelY1)/2;
panelZ1 = coverZ2 - 1 - panelThickness2;
panelZ2 = coverZ2 - 1 - panelThickness1;
panelZ3 = coverZ2 - 1;

// print display position for KiCad
echo(100+panelX1);
echo(100-panelY1);
echo(100+panelX2);
echo(100-panelY2);

// usb-c connector
usbX = 0;
usbZ = pcbZ2;
usbWidth = 9.0; // receptacle
usbHeight = 3.4;
usbConnectorWidth = 8.4; // connector
usbConnectorHeight = 2.5;
usbPlugWidth = 12; // plastic casing of connector
usbPlugHeight = 8;


// box with center/size in x/y plane and z ranging from z1 to z2
module box(x, y, w, h, z1, z2) {
	translate([x-w/2, y-h/2, z1])
		cube([w, h, z2-z1]);
}

module cuboid(x1, y1, x2, y2, z1, z2) {
	translate([x1, y1, z1])
		cube([x2-x1, y2-y1, z2-z1]);
}

module barrel(x, y, d, z1, z2) {
	translate([x, y, z1])
		cylinder(r=d/2, h=z2-z1);
}

module cone(x, y, d1, d2, z1, z2) {
	translate([x, y, z1])
		cylinder(r1=d1/2, r2=d2/2, h=z2-z1);
}

module frustum(x, y, w1, h1, w2, h2, z1, z2) {
	// https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#polyhedron	
	points = [
		// lower square
		[x-w1/2,  y-h1/2, z1],  // 0
		[x+w1/2,  y-h1/2, z1],  // 1
		[x+w1/2,  y+h1/2, z1],  // 2
		[x-w1/2,  y+h1/2, z1],  // 3
		// upper square
		[x-w2/2,  y-h2/2, z2],  // 4
		[x+w2/2,  y-h2/2, z2],  // 5
		[x+w2/2,  y+h2/2, z2],  // 6
		[x-w2/2,  y+h2/2, z2]]; // 7
	faces = [
		[0,1,2,3],  // bottom
		[4,5,1,0],  // front
		[7,6,5,4],  // top
		[5,6,2,1],  // right
		[6,7,3,2],  // back
		[7,4,0,3]]; // left  
	polyhedron(points, faces);
}


// reference parts
// ---------------

module usb() {
	color([0.5, 0.5, 0.5]) {
        translate([usbX, boxY1+2.1, usbZ+usbHeight/2]) {
            rotate([90, 0, 0]) {
                // recepacle
                hull() {
                    translate([-(usbWidth)/2, 0, 0])
                        cylinder(d=usbHeight, h=1);
                    translate([(usbWidth)/2, 0, 0])
                        cylinder(d=usbHeight, h=1);
                }

                // connector
                hull() {
                    translate([-usbConnectorWidth/2, 0, 0])
                        cylinder(d=usbConnectorHeight, h=10);
                    translate([usbConnectorWidth/2, 0, 0])
                        cylinder(d=usbConnectorHeight, h=10);
                }              
            }
        }
    }
}

module usbPlug() {
	usbPlugZ1 = (usbZ1+usbZ2)/2-usbPlugThickness/2;
	color([0, 0, 0])
	box(x=usbX, y=pcbY1-10, w=usbPlugWidth, h=14,
		z1=usbPlugZ1, z2=usbPlugZ1+usbPlugThickness);		
}

// printed circuit board
module pcb() {
	color([0, 0.6, 0, 0.3])
	box(x=pcbX, y=pcbY, w=pcbWidth, h=pcbHeight,
		z1=pcbZ1, z2=pcbZ2);
}

// digital potentiometer
module poti() {
	x = potiX;
	y = potiY;
	color([0.5, 0.5, 0.5]) {
		box(x, y, 13.4, 12.4, pcbZ2, potiZ1);
		box(x, y, 15, 6, pcbZ2, potiZ1);
		barrel(x, y, potiD1, pcbZ2, potiZ2);
		difference() {
			barrel(x, y, potiD2, pcbZ2, potiZ4);
			box(x, y+potiD2/2, 10, 3, potiZ3, potiZ4+0.5);
		}
	}
}

// 

module wheel() {
	x = potiX;
	y = potiY;

	// wheel
	difference() {
		// wheel
		barrel(x, y, wheelD2, wheelZ2, wheelZ3);

		// cutout for poti shaft (with tolerance)
		box(x=x, y=y, w=10, h=4.5, z1=1, z2=potiZ4+0.1);
		box(x=x, y=y, w=4.5, h=10, z1=1, z2=potiZ4+0.1);
	}
	
	// shaft holder for poti shaft
	difference() {
		intersection() {
			// box that has 1mm wall on sides
			box(x=x, y=y, w=8, h=8, z1=wheelZ1, z2=wheelZ2+1);

			// make round corners
			barrel(x, y, wheelD1, wheelZ1, wheelZ2+1);
		}

		// subtract rounded hole
		intersection() {
			box(x=x, y=y, w=potiD2, h=potiD2, z1=wheelZ1-1, z2=wheelZ3-1);
			barrel(x, y, 3.8*2, wheelZ1-1, wheelZ3-1);
		}
	}
	
	// poti shaft cutaway
	difference() {
		// fill shaft cutaway which is 1.5mm
		union() {
			box(x=x, y=y+2.5, w=1, h=2, z1=potiZ3, z2=wheelZ3-2);
			box(x=x, y=y+2.0, w=2, h=1, z1=potiZ3, z2=wheelZ3-2);
		}

		// subtract slanted corners to ease insertion of poti shaft
		translate([x, y+1.6, potiZ3-0.5])
			rotate([30, 0, 0])
				box(x=0, y=0, w=4, h=1, z1=0, z2=2);
	}
}

// for adding wheel base
module wheelBase(x, y) {
	// base in z-direction: 1mm air, 2mm wall
	barrel(x, y, wheelD2+(wheelGap+2)*2, wheelZ2-1-2, wheelZ3);
	
	// shaft radial: 5mm wheel shaft, air gap, 1mm wall
	// zhaft z-direction: 2mm wall, 1mm air, wheel
	barrel(x, y, wheelD1+(wheelGap+1)*2, wheelZ1-2-1, wheelZ3);

	// shaft radial: poti shaft, no air gap, 1.5mm wall
	barrel(x, y, potiD1+1.5*2, potiZ1, wheelZ3);
}

// for subtracting from wheel base
module potiCutout(x, y) {
	// cutout for wheel: 3mm wheel, 1mm air
	barrel(x, y, wheelD2+wheelGap*2, wheelZ2-1, coverZ2+1);

	// hole for poti shaft bearing
	barrel(x, y, potiD1+0.1*2, pcbZ2, wheelZ2);

	// hole for poti shaft
	barrel(x, y, potiD2+0.1*2, pcbZ2, wheelZ2);

	// hole for wheel shaft
	barrel(x, y, wheelD1+wheelGap*2, wheelZ1-1, wheelZ2);
}

// snap lock between cover and base
module snap(x, l) {
	translate([x, 0, coverZ1+1])
		rotate([0, 45, 0])
			cuboid(x1=-0.65, x2=0.65, y1=-l/2, y2=l/2, z1=-0.65, z2=0.65);
}

module drill(x, y, w, h) {
	box(x-100, 100-y, w+0.5, h+0.5, pcbZ2-4, pcbZ1);	
}

// generated from .drl files using drill2cad.py
// control
module drills() {
    drill(127.6, 122, 1.6, 3);
    drill(116.4, 122, 1.6, 3);
    drill(119.5, 115, 1, 1);
    drill(124.5, 115,1, 1);
    drill(119.5, 129.5, 1, 1);
    drill(122, 129.5, 1, 1);
    drill(124.5, 129.5, 1, 1);

    drill(76.73, 122, 0.9, 0.9);
    drill(79.27, 122, 0.9, 0.9);
}

module base() {
color([0.3, 0.3, 1]) {
	difference() {
		union() {
			difference() {
				// base
				union() {
					box(x=0, y=0, w=76-coverFit, h=76-coverFit,
						z1=0, z2=pcbZ1);
				
					// side walls for snap lock
					box(-(76-coverFit)/2+1, 0,
                        2, 76-coverFit,
						0, coverZ1+coverOverlap);
					box((76-coverFit)/2-1, 0,
                        2, 76-coverFit,
						0, coverZ1+coverOverlap);
                    
                    // upper wall
                    box(0, (76-coverFit)/2-1,
                        68, 2,
                        0, pcbZ2+0.5);
				}
				
				// subtract inner volume
				box(x=0, y=0, w=72-coverFit, h=72-coverFit, z1=2, z2=coverZ2);
			
				// subtract snap lock
				snap(-76/2, 44);
				snap(76/2, 44);			
			}
		}
		
		// subtract drills
		drills();

		// subtract mounting screw holes
		for (i = [-2 : 2]) {
			//cone(cos(i) * 60 - 30, sin(i) * 60, 2.8, 6, -0.1, 2.1);
			//cone(30 - cos(i) * 60, sin(i) * 60, 2.8, 6, -0.1, 2.1);
			cone(-30, i, 2.8, 6, -0.1, 2.1);
			cone(30, i, 2.8, 6, -0.1, 2.1);
			cone(i, -30, 2.8, 6, -0.1, 2.1);
			cone(i, 30, 2.8, 6, -0.1, 2.1);
		}

		// subtract cable hole
		box(0, 5, 50, 40, -1, pcbZ1);
	}

	// poti support
	barrel(potiX, potiY, 4, 0, pcbZ1);
} // color
}

module cover() {
color([1, 0, 0]) {
	difference() {
		union() {
            // box
            difference() {
                box(x=0, y=0, w=80, h=80, z1=coverZ1, z2=coverZ2);
                
                // subtract inner volume, leave 2mm wall
                box(x=0, y=0, w=76, h=76, z1=coverZ1-1, z2=coverZ2-2);
            }
			
            // poti base
			intersection() {
				wheelBase(x=potiX, y=potiY);
			
				// cut away poti base outside of cover and at display
				cuboid(x1=-40, y1=-40, x2=40, y2=panelY1, z1=pcbZ2, z2=coverZ2-2);
			}

			// photo diode border
			barrel(photoX, photoY,
				photoD+2,
				photoZ1, photoZ2);

            // pcb screw
            //barrel(pcbScrewX, pcbScrewY, M2Diameter+2*M2Wall, pcbZ2+1, pcbZ2+M2Depth+1);
            barrel(pcbScrewX, pcbScrewY, 4.9, pcbZ2, coverZ2);
			
			// lower display holder
			box(panelX, panelY1-0.25,
                panelWidth-4, 2,
				panelZ2-1.5, coverZ2);
		
			// upper display holder
			box(panelX, panelY2-0.5,
				10, 1.5,
				pcbZ2, panelZ1);
			box(panelX, panelY2+1, 10, 3, pcbZ2, pcbZ2+1.5);
            
			// display cable support
			box(0, -12, displayCableWidth1, 2,
				wheelZ2-3, coverZ2);
		}
		
		// subtract poti cutout for wheel and axis
		potiCutout(x=potiX, y=potiY);		

		// subtract photo diode cutout
		barrel(photoX, photoY,
			photoD,
			photoZ1-1, photoZ2+1);

		// subtract pcb screw hole
        //barrel(pcbScrewX, pcbScrewY, M2Diameter, pcbZ1, pcbZ2 + M2Depth);
        barrel(pcbScrewX, pcbScrewY, 2, pcbZ1, pcbZ1 + 12);
        
        // subtract display screen window
		frustum(x=screenX, y=screenY,
			w1=screenWidth-4, h1=screenHeight-4,
			w2=screenWidth+4, h2=screenHeight+4,
			z1=coverZ2-3, z2=coverZ2+1);

		// subtract display panel (front glass, smaller back glass not necessary)
		box(x=panelX, y=panelY, w=panelWidth, h=panelHeight,
			z1=panelZ2, z2=panelZ3);

		// subtract display cable cutout
		box(x=panelX, y=panelY1, w=displayCableWidth2, h=6,
			z1=panelZ3-10, z2=panelZ3+0.2);
		cuboid(x1=panelX1, y1=panelY1, x2=panelX2, y2=panelY1+5,
			z1=panelZ2, z2=panelZ3+0.2);
		frustum(x=screenX, y=panelY1, 
			w1=displayCableWidth1, h1=45,
			w2=displayCableWidth1, h2=6,
			z1=panelZ3-10, z2=panelZ3-0.5);
	
		// subtract components on pcb
		//usbPlug();
		usb();
	}		

	// snap lock between cover and base
	snap(-(76)/2, 40);
	snap((76)/2, 40);
} // color
}

// cover for production including wheel and programmer
module coverForProduction() {
	cover();
	
	// attached wheel
	box(0, -29.5, 1.5, 20, coverZ1, coverZ1+1.5);
	translate([-potiX, 6, coverZ1-wheelZ1]) {
		wheel();
	}
}


	
// casing parts that need to be printed
base();
cover();
//wheel();
//coverForProduction();

// reference parts
pcb();
//poti();
usb();
//usbPlug();
