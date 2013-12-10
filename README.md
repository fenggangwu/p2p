Fenggang Wu
4874734
fenggang@cs.umn.edu

Step 1: compile
make

Step 2:
xiang@cello (~) % mkdir /home/grad04/xiang/cello 
xiang@cello(~) % echo "this is mydoc1" > /home/grad04/xiang/cello/mydoc1 
xiang@saxophone (~) % mkdir /home/grad04/xiang/saxophone 
xiang@saxophone (~) % echo "this is mydoc2" > /home/grad04/xiang/saxophone/mydoc2 
xiang@oboe (~) % mkdir /home/grad04/xiang/oboe 
xiang@oboe (~) % echo "this is mydoc3" > /home/grad04/xiang/oboe/mydoc3 
xiang@trombone (~) % mkdir /home/grad04/xiang/trombone 
xiang@trombone (~) % echo "this is mydoc4" > /home/grad04/xiang/trombone /mydoc4


Step 3:
xiang@silver (~) % central-server 
xiang@cello (~) % peer /home/grad04/xiang/cello silver.cs.umn.edu
xiang@saxophone (~) % peer /home/grad04/xiang/saxophone silver.cs.umn.edu
xiang@oboe (~) % peer /home/grad04/xiang/oboe silver.cs.umn.edu
xiang@trombone (~) % peer /home/grad04/xiang/trombone silver.cs.umn.edu [


Step 4:
xiang@cello (~) % get mydoc4 
xiang@oboe (~) % share mydoc3 saxophone.cs.umn.edu

Step 5:
xiang@cello (~) % list
xiang@cello (~) % quit
