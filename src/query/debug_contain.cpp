/*
 * debug_distance.cpp
 *
 *  Created on: Dec 19, 2020
 *      Author: teng
 */

#include "../geometry/MyPolygon.h"
#include <fstream>
#include <boost/program_options.hpp>
#include "../geometry/triangulate.h"

namespace po = boost::program_options;
using namespace std;

int main(int argc, char **argv){
	MyPolygon *polygon=MyMultiPolygon::read_one_polygon();
	polygon->partition(10);
	Point p(-117.915111,33.799661);
	query_context ctx;
	ctx.use_grid = true;
	ctx.query_type = QueryType::contain;
	VertexSequence *ch = polygon->get_convex_hull();

	cout<<"1\n\n"<<ch->num_vertices-1<<endl;
	for(int i=0;i<ch->num_vertices-1;i++){
		printf("%f %f\n",ch->x[i],ch->y[i]);
	}
	return 0;
//	int *triangles = polygon->triangulate();
//	double *x = polygon->boundary->x;
//	double *y = polygon->boundary->y;
//	cout<<"MULTIPOLYGON(";
//	for(int i=0;i<polygon->get_num_vertices()-2;i++){
//		if(i!=0){
//			cout<<",";
//		}
//		printf("((%f %f,%f %f,%f %f,%f %f))",
//				x[triangles[3*i+0]],y[triangles[3*i+0]],
//				x[triangles[3*i+1]],y[triangles[3*i+1]],
//				x[triangles[3*i+2]],y[triangles[3*i+2]],
//				x[triangles[3*i+0]],y[triangles[3*i+0]]);
//	}
//	cout<<")"<<endl;



//	polygon->print();
//	query_context qt;
//	qt.use_grid = true;
	//polygon->print_partition(qt);
	//cout<<polygon->contain(p, &ctx)<<endl;
//
////
	int *triangles = new int[3*(polygon->get_num_vertices()-2)];

	triangulator *tr = new triangulator();
	tr->triangulate_polygon(polygon->boundary, triangles);

	for (int i = 0; i < polygon->get_num_vertices()-2; i++){
		printf("triangle #%d: %d %d %d\n", i, triangles[i*3+0], triangles[i*3+1], triangles[i*3+2]);
	}
	cout<<polygon->boundary->clockwise()<<endl;

	delete polygon;
	return 0;
}




