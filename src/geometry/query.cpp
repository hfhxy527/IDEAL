/*
 * query.cpp
 *
 *  Created on: Jun 2, 2020
 *      Author: teng
 */


#include "MyPolygon.h"
#include <float.h>
#include <math.h>


inline double point_to_segment_distance(const double x, const double y, const double x1, double y1, const double x2, const double y2) {

  double A = x - x1;
  double B = y - y1;
  double C = x2 - x1;
  double D = y2 - y1;

  double dot = A * C + B * D;
  double len_sq = C * C + D * D;
  double param = -1;
  if (len_sq != 0) //in case of 0 length line
      param = dot / len_sq;

  double xx, yy;

  if (param < 0) {
    xx = x1;
    yy = y1;
  } else if (param > 1) {
    xx = x2;
    yy = y2;
  } else {
    xx = x1 + param * C;
    yy = y1 + param * D;
  }

  double dx = x - xx;
  double dy = y - yy;
  return sqrt(dx * dx + dy * dy);
}

bool MyPolygon::contain(Point p){
    bool ret = false;
    for (int i = 0; i < get_num_vertices()-1; i++) {
        // segment i->j intersect with line y=p.y
        if ( ((gety(i)>p.y) != (gety(i+1)>p.y))){
            double a = (getx(i+1)-getx(i)) / (gety(i+1)-gety(i));
            if(p.x - getx(i) < a * (p.y-gety(i))){
                ret = !ret;
            }
        }
    }
    return ret;
}

bool MyPolygon::contain(Point p, query_context *ctx){

	if(!mbb){
		getMBB();
	}
	if(!mbb->contain(p)){
		return false;
	}
	if(ctx->query_type==QueryType::contain){
		ctx->checked_count++;
	}
	if(ctx&&ctx->use_grid&&is_grid_partitioned()){
		if(ctx->query_type==QueryType::contain){
			ctx->pixel_checked++;
		}
		struct timeval pixel_start = get_cur_time();
		int part_x = get_pixel_x(p.x);
		int part_y = get_pixel_y(p.y);
		if(ctx->query_type==QueryType::contain){
			ctx->pixel_check_time += get_time_elapsed(pixel_start);
		}
//		if(part_x>=partitions.size()){
//			log("%f %d %d %f %d %d",(p.x-mbb->low[0])/step_x,part_x,partitions.size(),(p.y-mbb->low[1])/step_y,part_y,partitions[0].size());
//		}
//		if(part_y>=partitions[0].size()){
//			log("%f %d %d %f %d %d %d",(p.x-mbb->low[0])/step_x,part_x,partitions.size(),(p.y-mbb->low[1])/step_y,part_y,partitions[0].size(),partitions[part_x][part_y].status);
//		}
		assert(part_x<partitions.size());
		assert(part_y<partitions[0].size());


		if(partitions[part_x][part_y].status==IN){
			ctx->raster_checked_only = true;
			return true;
		}
		if(partitions[part_x][part_y].status==OUT){
			ctx->raster_checked_only = true;
			return false;
		}


		ctx->raster_checked_only = false;

		bool ret = false;
		if(ctx->query_vector){
			struct timeval border_start = get_cur_time();
			assert(partitions[part_x][part_y].status==BORDER);
			for (int i = partitions[part_x][part_y].vstart; i < partitions[part_x][part_y].vend; i++) {
				// segment i->j intersect with line y=p.y
				if ( ((gety(i)>p.y) != (gety(i+1)>p.y))){
					double a = (getx(i+1)-getx(i)) / (gety(i+1)-gety(i));
					if(p.x - getx(i) < a * (p.y-gety(i))){
						ret = !ret;
					}
				}
			}
			if(ctx->query_type==QueryType::contain){
				ctx->edges_checked += partitions[part_x][part_y].vend-partitions[part_x][part_y].vstart;
				ctx->border_checked++;
				ctx->edges_check_time += get_time_elapsed(border_start);
			}
		}
		return ret;
	}else{
		if(ctx->query_type==QueryType::contain){
			ctx->edges_checked += get_num_vertices();
		}
        return contain(p);
	}
}


bool MyPolygon::contain_try_partition(Pixel *b, query_context *ctx){

	ctx->raster_checked_only = true;
	Pixel *a = getMBB();
	if(!a->contain(b)){
		return false;
	}
	// test all the pixels
	int txstart = this->get_pixel_x(b->low[0]);
	int tystart = this->get_pixel_y(b->low[1]);

	int incount = 0;
	int outcount = 0;
	int total = 0;

	double width_d = (b->high[0]-b->low[0]+this->step_x*0.9999999)/this->step_x;
	int width = double_to_int(width_d);

	double height_d = (b->high[1]-b->low[1]+this->step_y*0.9999999)/this->step_y;
	int height = double_to_int(height_d);

//	//log("%f",abs(b->high[0]-this->step_x*width-b->low[0]));
//
//	if(b->high[0]-this->step_x*width-b->low[0]<0.000001){
//		width--;
//	}
//

//	//log("%f",abs(b->high[1]-this->step_y*height-b->low[1]));
//
//	if(b->high[1]-this->step_y*height-b->low[1]<0.000001){
//		height--;
//	}
//
//	txend = txstart + width;
//	tyend = tystart + height;
//	if(txend==partitions.size()){
//		txend--;
//	}
//	if(tyend == partitions[0].size()){
//		tyend--;
//	}

	//log("%d\t%d",width,height);

	//log("%d %d %d %d %d %d",txstart, txend, txend-txstart, tystart,tyend,tyend-tystart);



	for(int i=txstart;i<txstart+width;i++){
		for(int j=tystart;j<tystart+height;j++){
			total++;
			if(partitions[i][j].status==OUT){
				outcount++;
			}
			if(partitions[i][j].status==IN){
				incount++;
			}
		}
	}
	// all is in
	if(incount==total){
		return true;
	}else if(outcount==total){
		return false;
	}
	//log("%d %d %d",total,incount,outcount);

	ctx->raster_checked_only = false;
	return false;
}


bool MyPolygon::contain(MyPolygon *target, query_context *ctx){
	// check each vertex in target
	for(int i=0;i<target->get_num_vertices();i++){
		Point p(target->getx(i),target->gety(i));
		if(!contain(p, ctx)){
			return false;
		}
	}
	return true;
}

bool MyPolygon::contain(Pixel *target, query_context *ctx){

	Pixel *a = getMBB();
	if(!a->contain(target)){
		return false;
	}
	if(ctx&&ctx->use_grid&&is_grid_partitioned()){
		// test all the pixels
		int txstart = this->get_pixel_x(a->low[0]);
		int txend = this->get_pixel_x(a->high[0]);
		int tystart = this->get_pixel_y(a->low[1]);
		int tyend = this->get_pixel_y(a->high[0]);
		int incount = 0;
		for(int i=txstart;i<=txend;i++){
			for(int j=tystart;j<=tyend;j++){
				if(partitions[i][j].status==OUT){
					return false;
				}
				if(partitions[i][j].status==IN){
					incount++;
				}
			}
		}
		// all is in
		if(incount==(txend-txstart+1)*(tyend-tystart+1)){
			return true;
		}
	}

	Point p1(target->low[0], target->low[1]);
	Point p2(target->high[0], target->high[1]);
	return contain(p1, ctx)&&contain(p2, ctx);

}

bool MyPolygon::intersect(MyPolygon *target, query_context *ctx){

	Pixel *a = getMBB();
	Pixel *b = target->getMBB();
	if(!a->intersect(b)){
		return false;
	}
	if(ctx&&ctx->use_grid&&is_grid_partitioned()){
		// test all the pixels
		int txstart = this->get_pixel_x(a->low[0]);
		int txend = this->get_pixel_x(a->high[0]);
		int tystart = this->get_pixel_y(a->low[1]);
		int tyend = this->get_pixel_y(a->high[0]);
		int outcount = 0;
		for(int i=txstart;i<=txend;i++){
			for(int j=tystart;j<=tyend;j++){
				if(partitions[i][j].status==OUT){
					outcount++;
				}else if(partitions[i][j].status==IN){
					return true;
				}
			}
		}
		// all is out
		if(outcount==(txend-txstart+1)*(tyend-tystart+1)){
			return false;
		}
	}

	// check each vertex in target
	for(int i=0;i<target->get_num_vertices();i++){
		Point p(target->getx(i),target->gety(i));
		if(contain(p, ctx)){
			return true;
		}
	}
	return false;
}

bool MyPolygon::intersect_segment(Pixel *target){
	for (int i = 0; i < get_num_vertices()-1; i++) {
		// segment i->j intersect with segment
		double x1 = target->low[0];
		double x2 = target->high[0];
		double y1 = target->low[1];
		double y2 = target->high[1];
		if ( ((gety(i)>y1) != (gety(i+1)>y1))){
			double a = (getx(i+1)-getx(i)) / (gety(i+1)-gety(i));
			double b = (getx(i)*gety(i+1)-getx(i+1)*gety(i))/(gety(i+1)-gety(i));
			double xc = a*y1+b;
			if(x1<xc!=x2<xc){
				return true;
			}
		}
		if ( ((gety(i)>y2) != (gety(i+1)>y2))){
			double a = (getx(i+1)-getx(i)) / (gety(i+1)-gety(i));
			double b = (getx(i)*gety(i+1)-getx(i+1)*gety(i))/(gety(i+1)-gety(i));
			double xc = a*y2+b;
			if(x1<xc!=x2<xc){
				return true;
			}
		}
		if ( ((getx(i)>x1) != (getx(i+1)>x1))){
			double a = (gety(i+1)-gety(i)) / (getx(i+1)-getx(i));
			double b = (gety(i)*getx(i+1)-gety(i+1)*getx(i))/(getx(i+1)-getx(i));
			double yc = a*x1+b;
			if(y1<yc!=y2<yc){
				return true;
			}
		}

		if ( ((getx(i)>x2) != (getx(i+1)>x2))){
			double a = (gety(i+1)-gety(i)) / (getx(i+1)-getx(i));
			double b = (gety(i)*getx(i+1)-gety(i+1)*getx(i))/(getx(i+1)-getx(i));
			double yc = a*x2+b;
			if(y1<yc!=y2<yc){
				return true;
			}
		}
	}
	return false;
}

double MyPolygon::distance(Point &p){
    double mindist = DBL_MAX;
    for (int i = 0; i < get_num_vertices()-1; i++) {
        double dist = point_to_segment_distance(p.x, p.y, getx(i), gety(i), getx(i+1), gety(i+1));
        if(dist<mindist){
            mindist = dist;
        }
    }
    return mindist;
}

double MyPolygon::distance(Point &p, query_context *ctx){

	// distance is 0 if contained by the polygon
	if(contain(p,ctx)){
		return 0;
	}

	ctx->checked_count++;

	if(ctx&&ctx->use_grid&&is_grid_partitioned()){

		Pixel *mbr = getMBB();
		double mbrdist = mbr->distance(p);

		//initialize the starting pixel
		Pixel *center_pixel = this->get_closest_pixel(p);
		const int start_pixx = center_pixel->id[0];
		const int start_pixy = center_pixel->id[1];
		int step = 0;
		double step_size = min(step_x,step_y);

		vector<Pixel *> needprocess;
		double mindist = 10000000.0;
		// for filtering
		int border_checked = 0;
		while(true){
			struct timeval pixel_start = get_cur_time();
			if(step==0){
				needprocess.push_back(center_pixel);
			}else{
				int xmin = max(0,start_pixx-step);
				int xmax = min((int)partitions.size()-1,start_pixx+step);
				int ymin = max(0,start_pixy-step);
				int ymax = min((int)partitions[0].size()-1,start_pixy+step);

				//left scan
				if(start_pixx-step>=0){
					for(int y=ymin;y<=ymax;y++){
						needprocess.push_back(&partitions[start_pixx-step][y]);
					}
				}
				//right scan
				if(start_pixx+step<partitions.size()){
					for(int y=ymin;y<=ymax;y++){
						needprocess.push_back(&partitions[start_pixx+step][y]);
					}
				}
				//bottom scan
				if(start_pixy-step>=0){
					for(int x=xmin+1;x<xmax;x++){
						needprocess.push_back(&partitions[x][start_pixy-step]);
					}
				}
				//top scan
				if(start_pixy+step<partitions[0].size()){
					for(int x=xmin+1;x<xmax;x++){
						needprocess.push_back(&partitions[x][start_pixy+step]);
					}
				}
			}
			ctx->pixel_check_time += get_time_elapsed(pixel_start);
			if(needprocess.size()==0){
				return distance(p);
			}
			ctx->pixel_checked += needprocess.size();

			for(Pixel *cur:needprocess){
				assert(cur->id[0]<partitions.size()&&cur->id[1]<partitions[0].size());
				//cur->print();
				//printf("checking pixel %d %d %d\n",cur->id[0],cur->id[1],cur->status);
				if(cur->status==BORDER){
					struct timeval border_start = get_cur_time();
					border_checked++;
					// no need to check the edges of this pixel
					if(ctx->query_type==QueryType::within){
						if(cur->distance_geography(p)>ctx->distance_buffer_size){
							continue;
						}else if(ctx->use_qtree){
							needprocess.clear();
							//grid indexing return
							return distance(p);
						}
					}
					// the vector model need be checked.
					ctx->border_checked++;
					ctx->edges_checked += cur->vend-cur->vstart;
					for (int i = cur->vstart; i <= cur->vend; i++) {
						double dist = point_to_segment_distance(p.x, p.y, getx(i), gety(i), getx(i+1), gety(i+1));
						if(dist<mindist){
							mindist = dist;
						}
					}
					ctx->edges_check_time += get_time_elapsed(border_start);
				}
			}
			needprocess.clear();

			//printf("step:%d radius:%f mindist:%f\n",step,mbrdist+step*step_size,mindist);

			// for within query, if all firstly processed boundary pixels
			// are not within the distance, it is for sure no edge will
			// be in the specified distance
			if(ctx->query_type==QueryType::within
					&&border_checked>0&&mindist==10000000.0){
				return mindist;
			}

			if(mindist<mbrdist+step*step_size){
				break;
			}

			step++;
		}
		// IDEAL return
		return mindist;

	}else{
		ctx->edges_checked += this->get_num_vertices();
		//SIMPVEC return
		return distance(p);
	}
}



//double MyPolygon::distance(Point &p, query_context *ctx){
//
//	if(contain(p,ctx)){
//		return 0;
//	}
//
//	ctx->checked_count++;
//
//	if(ctx&&ctx->use_grid&&is_grid_partitioned()){
//
//		Pixel *mbr = getMBB();
//
//		const double step = min(step_x,step_y)/2;
//		double radius = mbr->distance(p)+step;
//		double max_dist = mbr->max_distance(p);
//
//		double mindist = 10000000.0;
//		int border_checked = 0;
//		while(true){
//
//			//todo: for debugging only, should not happen
//			if(radius>max_dist){
//				return distance(p);
//				//cout<<iindex++<<endl;
//				return 0;
//				lock();
//				this->print();
//				this->print_partition(*ctx);
//				mbr->print();
//				p.print();
//				printf("max_dist:\t%f\nradius:\t%f\nstep_x:\t%f\ndimension_x:\t%ld\ndimension_y:\t%ld\n",
//						max_dist,radius,step,partitions.size(),partitions[0].size());
//				if(mbr->contain(p)){
//					int part_x = this->get_pixel_x(p.x);
//					int part_y = this->get_pixel_y(p.y);
//					printf("pixel_x:\t%d\npixel_y:\t%d\nstatus:\t%d\n",part_x, part_y,partitions[part_x][part_y].status);
//				}
//				exit(0);
//			}
//
//
//			double sx = 360;
//			double ex = -360;
//
//			// initialize the x range considering inside or outside the MBR
//			if(mbr->contain(p)){
//				sx = max(mbr->low[0],p.x-radius);
//				ex = min(mbr->high[0],p.x+radius);
//			}else{
//				if(mbr->high[1]>p.y-radius&&mbr->high[1]<p.y+radius){
//					double ty = sqrt(abs(radius*radius-(mbr->high[1]-p.y)*(mbr->high[1]-p.y)));
//					double xval = ty+p.x;
//					if(xval<mbr->high[0]&&xval>mbr->low[0]&&xval<sx){
//						sx = xval;
//					}
//					if(xval<mbr->high[0]&&xval>mbr->low[0]&&xval>ex){
//						ex = xval;
//					}
//					xval = p.x-ty;
//					if(xval<mbr->high[0]&&xval>mbr->low[0]&&xval<sx){
//						sx = xval;
//					}
//					if(xval<mbr->high[0]&&xval>mbr->low[0]&&xval>ex){
//						ex = xval;
//					}
//				}
//				if(mbr->low[1]>p.y-radius&&mbr->low[1]<p.y+radius){
//					double ty = sqrt(abs(radius*radius-(mbr->low[1]-p.y)*(mbr->low[1]-p.y)));
//					double xval = ty+p.x;
//					if(xval<mbr->high[0]&&xval>mbr->low[0]&&xval<sx){
//						sx = xval;
//					}
//					if(xval<mbr->high[0]&&xval>mbr->low[0]&&xval>ex){
//						ex = xval;
//					}
//					xval = p.x-ty;
//					if(xval<mbr->high[0]&&xval>mbr->low[0]&&xval<sx){
//						sx = xval;
//					}
//					if(xval<mbr->high[0]&&xval>mbr->low[0]&&xval>ex){
//						ex = xval;
//					}
//				}
//				if(mbr->low[0]>p.x-radius&&mbr->low[0]<p.x+radius){
//					double ty = sqrt(abs(radius*radius-(mbr->low[0]-p.x)*(mbr->low[0]-p.x)));
//					double yval = ty+p.y;
//					if(yval<mbr->high[1]&&yval>mbr->low[1]){
//						sx = mbr->low[0];
//					}
//					yval = p.y-ty;
//					if(yval<mbr->high[1]&&yval>mbr->low[1]){
//						sx = mbr->low[0];
//					}
//				}
//				if(mbr->high[0]>p.x-radius&&mbr->high[0]<p.x+radius){
//					double ty = sqrt(abs(radius*radius-(mbr->high[0]-p.x)*(mbr->high[0]-p.x)));
//					double yval = ty+p.y;
//					if(yval<mbr->high[1]&&yval>mbr->low[1]){
//						ex = mbr->high[0];
//					}
//					yval = p.y-ty;
//					if(yval<mbr->high[1]&&yval>mbr->low[1]){
//						ex = mbr->high[0];
//					}
//				}
//				if(p.y>mbr->low[1]&&p.y<mbr->high[1]){
//					if(p.x-radius>mbr->low[0]&&p.x-radius<sx){
//						sx = p.x-radius;
//					}
//					if(p.x+radius<mbr->high[0]&&p.x+radius>ex){
//						ex = p.x+radius;
//					}
//				}
//			}
//
//
//			if(sx<mbr->low[0]||sx>mbr->high[0]||ex<mbr->low[0]||ex>mbr->high[0]){
//				break;
//			}
//
//			// one iteration of pixel checking
//			for(double xval=sx;xval<=ex;xval+=step){
//				double ty = sqrt(abs(radius*radius-(xval-p.x)*(xval-p.x)));
//				int op = -1;
//				for(int t=0;t<(ty==0.0?1:2);t++){
//					double yval = p.y+ty*op;
//					op *= -1;
//					if(yval>mbr->high[1]||yval<mbr->low[1]){
//						continue;
//					}
//
//					int pixx = get_pixel_x(xval);
//					int pixy = get_pixel_y(yval);
//					//printf("radius:%f pixx:%d pixy:%d Point(%f %f)\n",radius,pixx,pixy,xval,yval);
//
//					if(partitions[pixx][pixy].status==BORDER){
//						border_checked++;
//						// no need to check the edges of this pixel
//						if(ctx->query_type==QueryType::within){
//							if(partitions[pixx][pixy].distance_geography(p)>ctx->distance_buffer_size){
//								ctx->raster_checked++;
//								continue;
//							}else if(ctx->use_qtree){//grid indexing
//								ctx->vector_checked++;
//								ctx->edges_checked += this->get_num_vertices();
//								return distance(p);
//							}
//						}
//						ctx->vector_checked++;
//						for (int i = partitions[pixx][pixy].vstart; i <= partitions[pixx][pixy].vend; i++) {
//							double dist = point_to_segment_distance(p.x, p.y, getx(i), gety(i), getx(i+1), gety(i+1));
//							if(dist<mindist){
//								mindist = dist;
//							}
//							ctx->edges_checked++;
//						}
//					}else{
//						ctx->raster_checked++;
//					}
//				}
//
//			}
//			// for within query, if all firstly processed boundary pixels
//			// are not within the distance, it is for sure no edge will
//			// be in the specified distance
//			if(ctx->query_type==QueryType::within){
//			    if(border_checked>0&&mindist==10000000.0){
//                    return mindist;
//                }
//			}
//
//			//minimum distance is found
//			if(mindist<=radius){
//				return mindist;
//			}
//			//otherwise, enlarge the buffer circle
//			radius += step;
//
//
//		}
//		return mindist;
//	}else{//SIMPVEC
//
//		ctx->vector_checked++;
//		ctx->edges_checked += this->get_num_vertices();
//		return distance(p);
//	}
//}



