//$fn=255;

tollerance = 0.1;
margin=3;
base_height=3;

screw=5;

module ring(inner_diameter, outer_diameter, height){
    translate([0,0,base_height])
    linear_extrude(height + 2 * tollerance){
        difference(){
            circle(d=(outer_diameter + 2*tollerance));
            circle(d=(inner_diameter - 2 * tollerance));
        }
    }
}

module base(inner_diameter, outer_diameter, height) {
    difference(){ 
        linear_extrude(height + base_height + tollerance * 2)
        circle(r=(outer_diameter/2 + margin * 2 + screw));
        
        translate([0, 0, base_height])
        linear_extrude(height + 2 * tollerance + base_height)
        circle(d=(outer_diameter + 2 * tollerance));
        
        for(i=[0:90:360]){
            rotate(i){                
                translate([outer_diameter/2 + margin + screw/2, 0, 0])
                linear_extrude(height * 3)
                circle(d=(screw+tollerance));            

                translate([outer_diameter/2 + margin + screw /2, 0, height])
                linear_extrude(height * 3)
                circle(d=(screw+tollerance+2));            
            }
        }
                
        linear_extrude(height*10,center=true){
            circle(d=(outer_diameter - 1));        
        }
        
            
        linear_extrude(height*10,center=true){
            circle(d=(inner_diameter - margin));
        }
    }          
}

module top(inner_diameter, outer_diameter, height) {
    difference(){
        translate([0,0,2*(height+base_height)+1])
        mirror([0,0,1]) 
        union(){
            difference(){ 
                linear_extrude(height + base_height + tollerance * 2)
                circle(r=(outer_diameter/2 + margin * 2 + screw));
                            
               for(i=[0:90:360]){
                    rotate(i){       
              
                        // outer screws         
                        translate([outer_diameter/2 + margin + screw/2, 0, 0])
                        linear_extrude(height * 3)
                        circle(d=(screw+tollerance));            

                        translate([outer_diameter/2 + margin + screw /2, 0, height])
                        linear_extrude(height * 3)
                        circle(d=(screw+tollerance+2));      

                        // inner screws
                        translate([inner_diameter/2 - margin - screw/2, 0, 0])
                        linear_extrude(height * 3)
                        circle(d=(screw+tollerance));                  
                     
                    }
                }
                        
            }          
                  
            difference(){   
            translate([0,0,height + base_height]){
                linear_extrude(height+tollerance+2)
                circle(d=(inner_diameter - tollerance));

                linear_extrude(2)
                circle(d=(inner_diameter + (outer_diameter-inner_diameter)/2));
            
            }
                   for(i=[0:90:360]){
                    rotate(i){                            // inner screws
                        translate([inner_diameter/2 - margin - screw/2, 0, 0])
                        linear_extrude(height * 3)
                        circle(d=(screw+tollerance)); 
            }
        }
    }
        }
        
        
        
        
        
        linear_extrude(height*10,center=true){
            circle(d=(inner_diameter/2));
        }

    }
}


module topHat(inner_diameter, outer_diameter, height){
    translate([0,0,-height - base_height])
     difference(){   
            translate([0,0,height + base_height]){
                
                linear_extrude(2)
                circle(d=(inner_diameter + (outer_diameter-inner_diameter)/2));
            
            }
                   for(i=[0:90:360]){
                    rotate(i){                            // inner screws
                        translate([inner_diameter/2 - margin - screw/2, 0, 0])
                        linear_extrude(height * 3)
                        circle(d=(screw+tollerance)); 
            }
        }
        
        linear_extrude(height*10,center=true){
            circle(d=(inner_diameter/2));
        }
        }
    }
///////////////////////////////

module ring_50_65(){
    inner_diameter = 50;
    outer_diameter = 65;
    height = 7;
    ring(inner_diameter, outer_diameter, height);
}

module base_50_65(){
    inner_diameter = 50;
    outer_diameter = 65;
    height = 7;
    base(inner_diameter, outer_diameter, height);
}

module top_50_65(){
    inner_diameter = 50;
    outer_diameter = 65;
    height = 7;
    top(inner_diameter, outer_diameter, height);
}

module topHat_50_65(){
    inner_diameter = 50;
    outer_diameter = 65;
    height = 7;
    topHat(inner_diameter, outer_diameter, height);
}

///////////////////////////////

module ring_10_26(){
    inner_diameter = 10;
    outer_diameter = 26;
    height = 8;
    ring(inner_diameter, outer_diameter, height);
}

module base_10_26(){
    inner_diameter = 10;
    outer_diameter = 26;
    height = 8;
    base(inner_diameter, outer_diameter, height);
}

module top_10_26(){
    inner_diameter = 10;
    outer_diameter = 26;
    height = 8;
    top(inner_diameter, outer_diameter, height);
}

///////////////////////////////

module ring_7_14(){
    inner_diameter = 7;
    outer_diameter = 14;
    height = 5;
    ring(inner_diameter, outer_diameter, height);
}

module base_7_14(){
    inner_diameter = 7;
    outer_diameter = 14;
    height = 5;
    base(inner_diameter, outer_diameter, height);
}

module top_7_14(){
    inner_diameter = 7;
    outer_diameter = 14;
    height = 5;
    top(inner_diameter, outer_diameter, height);
}

///////////////////////////////


//color("lightgreen")
//base_10_26();
//base_7_14();

//translate([0,0,base_height])
//color("pink")
//ring_10_26();


//ring_7_14();
//base_10_26();


translate([0,0,0]){
    color("yellow") base_50_65();
   color("gray") ring_50_65();
    color("lightgreen") top_50_65();
    color("lightblue") topHat_50_65();
}

/*
translate([80,0,0]){
    color("lightgreen") base_10_26();
    color("lightblue") ring_10_26();
    color("pink") top_10_26();
}
*/

/*
translate([130,0,0]){
    color("lightgreen") base_7_14();
    color("lightblue") ring_7_14();
    color("pink") top_7_14();
}
*/