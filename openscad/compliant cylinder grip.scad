$fn=255;


module grip(h=10, d=10, o=1, tollerance=0.1){
    union(){
        difference(){
            children();
            linear_extrude(h+1,center=true){
                offset(-tollerance) circle(d=d);
            }
            
            difference(){
                linear_extrude(h+1,center=true){
                    offset(2*o) circle(d=d);
                }
                linear_extrude(h+2,center=true){
                    offset(o) circle(d=d);
                }
            }

            intersection(){
                linear_extrude(h+1,center=true){
                    offset(o-tollerance) circle(d=d);
                }
                for(i=[0:60:360]){
                    rotate([0,0,i+30]){
                        translate([h/2,0,0]) cube([h+1,o,h+1],center=true);
                        rotate([0,0,30]){
                            scale([1,d/(d+tollerance),1]) {linear_extrude(h+1, center=true) {
                                offset(+tollerance) circle(d=d);
                                }
                            }
                        }
                    }
                }                
            }
        }
        intersection(){
            difference(){
                linear_extrude(h+1,center=true){
                    offset(3*o) circle(d=d);
                }
                linear_extrude(h+2,center=true){
                    offset(o) circle(d=d);
                }
            }
            for(i=[0:60:360]){
                    rotate([0,0,i]){
                        translate([h/2,0,0]) cube([h,o,h],center=true);
                    }
                }
            }
    }
}

h=20;
grip(h=h, o=1, d=8) linear_extrude(h, center=true) circle(d=16);
