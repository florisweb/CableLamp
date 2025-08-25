
bottomThickness = 1.5;

width = 161;
height = 17;
depth = 158;
ribDepth = 2;
attachementCylinderRadMin = 4.4;
attachementCylinderRadMax = 4.52;
attachementBottomOffset = 2;
attachementDepth = 14;

cableCutOutRadius = 30;

body();


module body() {
    difference() {
        union() {
            cube([width, ribDepth, height]);

            translate([0, 0, height - bottomThickness])
            cube([width, depth, bottomThickness]);
    
            
            translate([0, ribDepth, height - bottomThickness]) {
                translate([5, 0, 0])
                rib(depth - ribDepth, height - bottomThickness, ribDepth);

                translate([(width - ribDepth) / 2, 0, 0])
                rib(depth - ribDepth, height - bottomThickness, ribDepth);

                translate([width - ribDepth - 5, 0, 0])
                rib(depth - ribDepth - cableCutOutRadius, height - bottomThickness, ribDepth);
            }
            
            


            translate([16, 0, attachementBottomOffset + attachementCylinderRadMax])
            rotate([90, 0, 0])
            cylinder(r1=attachementCylinderRadMax,r2=attachementCylinderRadMin, h=attachementDepth);

            translate([79.3, 0, attachementBottomOffset + attachementCylinderRadMax])
            rotate([90, 0, 0])
            cylinder(r1=attachementCylinderRadMax,r2=attachementCylinderRadMin, h=attachementDepth);

            translate([144, 0, attachementBottomOffset + attachementCylinderRadMax])
            rotate([90, 0, 0])
            cylinder(r1=attachementCylinderRadMax,r2=attachementCylinderRadMin, h=attachementDepth);
        }
        translate([width, depth, 0])
        cylinder(r=cableCutOutRadius, h=height);

    }
}
module rib(length, height, thickness) {
    rotate([0, 90, 0])
    linear_extrude(thickness)
    polygon(points=[[0,0],[height,0],[0,length]], paths=[[0,1,2]]);
//    cube([ribDepth, depth, height]);
}