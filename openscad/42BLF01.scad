
$fn=255;

module ACT42BLF01(){
    module body(){
        intersection(){
            difference() {
                translate([-21,-21,-47]) cube([42,42,47]);
                for(i=[0:90:360]) rotate(i) translate([15.5,15.5,-4]) linear_extrude(5) circle(d=3);
                for(i=[0:90:360]) rotate(i) translate([15.5,15.5,-48]) linear_extrude(5) circle(d=3);
                translate([0,0,-47.1]) linear_extrude(0.2) circle(d=7);
            }
            rotate([0,0,45]) cube([38*sqrt(2),38*sqrt(2),100], center=true);
        }
        
    }

    difference(){
        union(){
            color("grey") difference(){
                body();
                translate([0,0,-10])scale([1.1,1.1,.5]) body();
            }
            color("black") translate([0,0,-10])scale([.99,.99,.5]) body();
            color("grey") difference(){
                linear_extrude(2) circle(d=22);            
                translate([0,0,1.9]) linear_extrude(0.2) circle(d=7);
            }
        }
        color("black") translate([0,0,-52]) linear_extrude(60) circle(d=5.1);
    }
    

    color("grey") translate([0,0,-47]) linear_extrude(24+47) circle(d=5);
}

ACT42BLF01();


