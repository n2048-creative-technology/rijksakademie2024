/*
https://www.youtube.com/watch?v=jQ6LQBFZXmU

https://www.tec-science.com/mechanical-power-transmission/planetary-gear/how-does-a-cycloidal-gear-drive-work/

*/
use <rings.scad>;
use <42BLF01.scad>;

$fn=360;

ratio=20; // 10:1 ratio

//pin_dia=8;
//inner_dia = (ratio*pin_dia);

total_dia = 65;
margin = 10;

/*
inner_dia = total_dia/(1+1/ratio)-margin;
pin_dia = inner_dia/ratio; // 2,5952380952 mm
*/

pin_dia = 2.6;  
inner_dia = pin_dia * ratio;
    
    
num_pins = ratio+1;

bearing_height = 8;
bearing_in_dia = 10;
bearing_out_dia = 26;

eccentricity = pin_dia/2; //4 mm


 module cycloid(rotation=0, tollerance=0.2){
     rotate([0,0,rotation]){
         r1=inner_dia/2;
         r2=inner_dia/ratio/2;
         angles=[ for (i = [0:1:360]) i ];
         coords=[ for (a=angles) [
                cos(a)*(r1+r2) + cos(a*(1+ratio))*r2,
                
                sin(a)*(r1+r2) + sin(a*(1+ratio))*r2
             ] ];
         offset(-pin_dia/2 - tollerance)
         polygon(coords);
     }
 }



module pins(height=10){ 
    // pins // negative space: replaced by metal rods:
    for(i=[0:360/(num_pins):360]){
        rotate([0,0,i]){
//            translate([inner_dia*(1.+1./(ratio))/2,0,0]){
            translate([(inner_dia+pin_dia)/2,0,0]){
                linear_extrude(height){
                    circle(d=pin_dia);
                }
            }
        }
    }
}


module housing(base_height=3){
    linear_extrude(base_height){
        circle(d=inner_dia+pin_dia+2);
    }
    if($children>0) {
        translate([0,0,base_height]) children();
    }
}


* housing() {
    color("red") pins();
    
    color("cyan") translate([-inner_dia/ratio/2,0,1]){
        linear_extrude(bearing_height) 
            cycloid(tollerance-0);
    }
}



module closedHouse(height=10, tollerance=0.1){
    difference(){
        linear_extrude(height, center=true) circle(d=inner_dia+pin_dia+3*margin);
        for(i=[0:1:num_pins]){
            r=i*360/num_pins;
            linear_extrude(height+1, center=true) offset(tollerance) translate([-pin_dia/2*cos(r),pin_dia/2*sin(r),0]) cycloid(r/ratio, tollerance=0);
        }
        
        
        for(i=[0:60:360]) rotate([0,0,i]) translate([37.1,0,0]) { 
            linear_extrude(100, center=true) circle(d=4);
            translate([0,0,19]) cylinder(h=3,d1=4,d2=6);
        }        
        for(i=[0:60:360]) rotate([0,0,i+30]) {
           translate([42,0,0]) cube([2,2,100], center=true);
        }
        
        
        for(j=[0:60:360]){ 
            for(i=[0:1:30]) rotate([0,0,j+i+15]) {
                translate([35,0,0]) linear_extrude(100, center=true) circle(d=5);
            }
        }
        
        translate([0,0,height/2]) linear_extrude(2, center=true) circle(d=63);
        translate([0,0,height/2-1]) linear_extrude(2, center=true) circle(d=60);
        
        
    }
}

module closedHouseBottom(height=10){
   translate([0,0,-(10+height)/2]) {      
       intersection(){
            difference(){
                linear_extrude(10, center=true) circle(d=inner_dia+pin_dia+3*margin);
                linear_extrude(20, center=true) circle(d=26);
                
                translate([0,0,-8]) linear_extrude(10,center=true) offset(0.2) projection(cut=false) ACT42BLF01();
                for(i=[0:90:360]) rotate(i) translate([15.5,15.5,-4]) linear_extrude(50,center=true) circle(d=3.5);
                translate([0,0,-10]) linear_extrude(20, center=true) circle(d=10);
                
                for(i=[0:60:360]) rotate([0,0,i]) translate([37.1,0,0]) { 
                    linear_extrude(100, center=true) circle(d=4);
                    translate([0,0,19]) cylinder(h=3,d1=4,d2=6);
                }            
                for(i=[0:60:360]) rotate([0,0,i+30]) {
                   translate([42,0,0]) cube([2,2,100], center=true);
                }
            }    
            translate([0,0,-34.5]) cylinder(h=50,d1=45,d2=110);   
        } 
    }
    
}

module closedHouseTop(height=10){
    intersection(){
        difference(){
           translate([0,0,15]) {     
                difference(){
                    linear_extrude(10, center=true) circle(d=inner_dia+pin_dia+3*margin);
                    linear_extrude(40, center=true) circle(d=65);    
                }
                translate([0,0,-5]) {
                    difference(){
                        translate([0,0,9]) linear_extrude(2, center=true) circle(d=inner_dia+pin_dia+3*margin);
                       linear_extrude(40, center=true) circle(d=63);  
                    }
                }
            }
            cylinder(h=40,d1=27,d2=100);
            for(i=[0:60:360]) rotate([0,0,i]) translate([37.1,0,0]) { 
                linear_extrude(100, center=true) circle(d=4);
                translate([0,0,19]) cylinder(h=3,d1=4,d2=6);
            }
            for(i=[0:60:360]) rotate([0,0,i+30]) {
               translate([42,0,0]) cube([2,2,100], center=true);
            }
        }
        cylinder(h=50,d1=110,d2=44);
    }
}

module motorShaft(height=10, d=5, rotation=0, l=40, withRod=false){
    rotate( [0,0,rotation]) {
        difference(){
            union(){
                translate([-eccentricity*cos(-rotation),eccentricity*sin(-rotation),height/2]){
                    linear_extrude(9, center=true) circle(d=10);
                    translate([0,0,-4.5]) linear_extrude(1, center=true) circle(d=12);
                }
                translate([-eccentricity*cos(-rotation+180),eccentricity*sin(-rotation+180),-height/2]) {
                    linear_extrude(9, center=true) circle(d=10);
                    translate([0,0,4.5]) linear_extrude(1, center=true) circle(d=12);
                }
            }
            linear_extrude(l, center=true) circle(d=d);
            cube([1.2,7,l], center=true);
        }
        if (withRod) linear_extrude(l, center=true) circle(d=d);    
    }
}

module shaftCenter(l=8, d=5){
    difference(){        
        linear_extrude(l, center=true) circle(d=10);
            linear_extrude(l+1, center=true) circle(d=d);
            cube([1.2,7,l+1], center=true);
    }
    
}

module outputShaft(d=5, rotation=0){
    rotate( [0,0,rotation]) {
        //for(i=[0:90:360]) rotate([0,0,i]) translate([19,0,-12]) linear_extrude(20, center=true) circle(d=pin_dia);
        intersection(){
            union(){
                difference(){
                    union(){
                        translate([0,0,-4.5]) linear_extrude(2, center=true) circle(d=52);
                        linear_extrude(11, center=true) circle(d=50);
                    }
                    linear_extrude(50, center=true) circle(d=26);
                    for(i=[0:90:360]) {
                        rotate([0,0,i+45]) translate([20,0,2]) {
                            linear_extrude(10,center=true) circle(d=3.5);
                            translate([0,0,2]) cylinder(h=2,d1=3,d2=4);
                        }
                        rotate([0,0,i]) translate([19,0,0]) linear_extrude(50, center=true) circle(d=pin_dia);
                    }
                   translate([0,0,4]) cylinder(h=2,d1=26,d2=28);
                }
                difference(){
                    translate([0,0,-4.5]) linear_extrude(2, center=true) circle(d=26);
                    linear_extrude(50, center=true) circle(d=24);
                    translate([0,0,-4]) cylinder(h=2,d1=24,d2=26);
                }
            }
            cylinder(h=50,d1=100,d2=16, center=true);
        }
    }
}

module animate(height=10, r=900){
    r=$t*3600;
    
     color("orange") closedHouseBottom(height=height*2);
    # color("yellow") closedHouse(height=height*2);
    # color("orange") closedHouseTop(height=height*2);
    
    
    color("grey")translate([0,0,9]) ring_50_65();
    
    translate([0,0,16]) outputShaft(rotation=r/ratio);
    
    
    union(){
        color("blue") motorShaft(height=height,rotation=-r);    
        color("orange") translate([0,0,height/2]) disk(8,r);
        color("blue") translate([0,0,-height/2]) disk(8,r+180);
    }
    
    color("grey") translate([-eccentricity*cos(r),eccentricity*sin(r),height/2-7]) ring_10_26();
    color("grey") translate([-eccentricity*cos(r+180),eccentricity*sin(r+180),-height/2-7]) ring_10_26();
    
    
   
    // translate([0,0,-18]) ACT42BLF01();
}

module disk(height=10, r=0){
//    translate([-eccentricity*cos(r),eccentricity*sin(r),0]){
    translate([-eccentricity*cos(r),eccentricity*sin(r),0]){
    difference(){        
            linear_extrude(height, center=true) cycloid(r/ratio);    
            linear_extrude(height+1, center=true) circle(d=26);
            
            rotate([0,0,r/ratio]) for(i=[0:90:360]){
                rotate([0,0,i])
                translate([19,0,0]){
                    linear_extrude(height+1, center=true) circle(d=2*(pin_dia+eccentricity));
                }
            }
        }
    }
}


module part1() closedHouse(height=20);
module part2() closedHouseBottom(height=20);
module part3() closedHouseTop(height=20);
module part4() outputShaft(withRod=false);
module part5() motorShaft(height=10);
module part6() disk(8);
module part7() shaftCenter();

module print(part=0) {
  //if (part==0) animate();
  if (part==1) part1();
  if (part==2) part2();
  if (part==3) part3();
  if (part==4) part4();
  if (part==5) part5();
  if (part==6) part6();
  if (part==7) part7();
}

part = 0; 
print(part);

