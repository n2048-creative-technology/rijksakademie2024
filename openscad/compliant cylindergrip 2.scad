$fn = 255;


module spiral(dist=2, r=1, thick=1, h=3, deg=360){
    points1=[for(i=[0:deg])let(r=r+i/360*dist)[cos(i),sin(i)]*r];
    points2=[for(i=[deg:-1:0])let(r=r-thick+i/360*dist)[cos(i),sin(i)]*r];
    linear_extrude(h, center=true) polygon(concat(points1,points2));
}



module grip(h=10, d=10, o=1, tollerance=0.1){

    intersection(){
        difference(){
           linear_extrude(h+1, center=true) offset(2) circle(d=d);
           linear_extrude(h+2, center=true) offset(-tollerance) circle(d=d);
        }

        for(i=[0:10:360]){
            rotate([0,0,i]){
                intersection(){
                    linear_extrude(h, center=true) offset(4) circle(d=d);
                    spiral(r=d/2,dist=3*d,thick=0.5, h=h+1);
                }
            }
        }
    }
    
     difference(){
       linear_extrude(h, center=true) offset(4) circle(d=d);
       linear_extrude(h+1, center=true) offset(2) circle(d=d);
    }
}

h = 10;
d = 10;

grip(d=d, h=h); 

*color("red") linear_extrude(h, center=true) circle(d=d);