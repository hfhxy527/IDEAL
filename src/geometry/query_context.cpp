/*
 * query_context.cpp
 *
 *  Created on: Sep 3, 2020
 *      Author: teng
 */
#include "MyPolygon.h"

query_context::query_context(){
	num_threads = get_num_threads();
	pthread_mutex_init(&lock, NULL);
}
query_context::~query_context(){

	vertex_number.clear();
	latency.clear();
	if(this->global_ctx == NULL){
		for(MyPolygon *p:source_polygons){
			delete p;
		}
		for(MyPolygon *p:target_polygons){
			delete p;
		}
		if(points){
			delete []points;
		}
	}

}

query_context::query_context(query_context &t){
	thread_id = t.thread_id;
	num_threads = t.num_threads;
	vpr = t.vpr;
	vpr_end = t.vpr_end;
	use_grid = t.use_grid;
	use_qtree = t.use_qtree;
	use_mer = t.use_mer;
	use_triangulate = t.use_triangulate;
	mer_sample_round = t.mer_sample_round;
	use_convex_hull = t.use_convex_hull;
	perform_refine = t.perform_refine;
	gpu = t.gpu;
	sample_rate = t.sample_rate;
	small_threshold = t.small_threshold;
	big_threshold = t.big_threshold;
	sort_polygons = t.sort_polygons;
	report_gap = t.report_gap;
	distance_buffer_size = t.distance_buffer_size;
	source_path = t.source_path;
	target_path = t.target_path;
	target_num = t.target_num;

	valid_path = t.valid_path;
	query_type = t.query_type;
	collect_latency = t.collect_latency;
	pthread_mutex_init(&lock, NULL);
}

query_context& query_context::operator=(query_context const &t){
	thread_id = t.thread_id;
	num_threads = t.num_threads;
	vpr = t.vpr;
	vpr_end = t.vpr_end;
	use_grid = t.use_grid;
	use_qtree = t.use_qtree;
	use_mer = t.use_mer;
	use_triangulate = t.use_triangulate;
	mer_sample_round = t.mer_sample_round;
	use_convex_hull = t.use_convex_hull;
	perform_refine = t.perform_refine;
	gpu = t.gpu;
	sample_rate = t.sample_rate;
	small_threshold = t.small_threshold;
	big_threshold = t.big_threshold;
	sort_polygons = t.sort_polygons;
	report_gap = t.report_gap;
	distance_buffer_size = t.distance_buffer_size;
	source_path = t.source_path;
	target_path = t.target_path;
	valid_path = t.valid_path;
	target_num = t.target_num;

    query_type = t.query_type;
    collect_latency = t.collect_latency;
    pthread_mutex_init(&lock, NULL);
	return *this;

}

void query_context::report_latency(int num_v, double lt){
	if(vertex_number.find(num_v)==vertex_number.end()){
		vertex_number[num_v] = 1;
		latency[num_v] = lt;
	}else{
		vertex_number[num_v] = vertex_number[num_v]+1;
		latency[num_v] = latency[num_v]+lt;
	}
}

void query_context::load_points(){
	struct timeval start = get_cur_time();
	long fsize = file_size(target_path.c_str());
	if(fsize<=0){
		log("%s is empty",target_path.c_str());
		exit(0);
	}else{
		log("size of %s is %ld",target_path.c_str(),fsize);
	}
	target_num = fsize/(2*sizeof(double));

	points = new double[target_num*2];
	ifstream infile(target_path.c_str(), ios::in | ios::binary);
	infile.read((char *)points, fsize);
	infile.close();
	logt("loaded %ld points", start,target_num);
}

void query_context::report_progress(){
	if(++query_count==100){
		pthread_mutex_lock(&global_ctx->lock);
		global_ctx->query_count += query_count;
		double time_passed = get_time_elapsed(global_ctx->previous);
		if(time_passed/1000>global_ctx->report_gap){
			log("processed %d (%.2f\%)",global_ctx->query_count,(double)global_ctx->query_count*100/(global_ctx->target_num*global_ctx->sample_rate));
			global_ctx->previous = get_cur_time();
		}
		query_count = 0;
		pthread_mutex_unlock(&global_ctx->lock);
	}
}

void query_context::merge_global(){
	pthread_mutex_lock(&global_ctx->lock);
	global_ctx->found += found;
	global_ctx->query_count += query_count;
	global_ctx->refine_count += refine_count;


	global_ctx->object_checked += object_checked;
	global_ctx->pixel_evaluated += pixel_evaluated;
	global_ctx->border_evaluated += border_evaluated;
	global_ctx->border_checked += border_checked;
	global_ctx->edge_checked += edge_checked;
	global_ctx->intersection_checked += intersection_checked;

	for(auto &it :vertex_number){
		const double lt = latency.at(it.first);
		if(global_ctx->vertex_number.find(it.first)!=global_ctx->vertex_number.end()){
			global_ctx->vertex_number[it.first] = global_ctx->vertex_number[it.first]+it.second;
			global_ctx->latency[it.first] = global_ctx->latency[it.first]+lt;
		}else{
			global_ctx->vertex_number[it.first] = it.second;
			global_ctx->latency[it.first] = lt;
		}
	}
	pthread_mutex_unlock(&global_ctx->lock);
}

bool query_context::next_batch(int batch_num){
	pthread_mutex_lock(&global_ctx->lock);
	if(global_ctx->index==global_ctx->target_num){
		pthread_mutex_unlock(&global_ctx->lock);
		return false;
	}
	index = global_ctx->index;
	if(index+batch_num>global_ctx->target_num){
		index_end = global_ctx->target_num;
	}else {
		index_end = index+batch_num;
	}
	global_ctx->index = index_end;
	pthread_mutex_unlock(&global_ctx->lock);
	return true;
}

void query_context::print_stats(){

	log("query count:\t%ld",query_count);
	log("checked count:\t%ld",object_checked.counter);
	log("found count:\t%ld",found);

	if(object_checked.counter>0){
		if(refine_count)
		log("refine/checked:\t%f",(double)refine_count/object_checked.counter);
		if(pixel_evaluated.counter)
		log("pixel/checked:\t%f",(double)pixel_evaluated.counter/object_checked.counter);
		if(border_checked.counter)
		log("border/checked:\t%f",(double)border_checked.counter/object_checked.counter);
		if(edge_checked.counter)
		log("checked/edges:\t%f",(double)edge_checked.counter/object_checked.counter);
	}

	if(border_checked.counter>0){
		log("border/edges:\t%f",(double)edge_checked.counter/border_checked.counter);
		log("node/border:\t%f",(double)intersection_checked.counter/border_checked.counter);
	}

	if(pixel_evaluated.counter>0){
		log("latency/pixel:\t%f",pixel_evaluated.execution_time/pixel_evaluated.counter);
	}
	if(border_evaluated.counter>0){
		log("latency/border:\t%f",border_evaluated.execution_time/border_evaluated.counter);
	}
	if(edge_checked.execution_time>0){
		log("latency/edge:\t%f",edge_checked.execution_time/edge_checked.counter);
	}
	if(intersection_checked.execution_time>0){
		log("latency/node:\t%f",intersection_checked.execution_time/intersection_checked.counter);
	}
	if(object_checked.execution_time>0){
		log("latency/other:\t%f",(object_checked.execution_time-pixel_evaluated.execution_time-border_evaluated.execution_time-edge_checked.execution_time)/object_checked.counter);
	}

	if(collect_latency){
		for(auto it:vertex_number){
			cout<<it.first<<"\t"<<latency[it.first]/it.second<<endl;
		}
	}
}


query_context get_parameters(int argc, char **argv){
	query_context global_ctx;

	po::options_description desc("query usage");
	desc.add_options()
		("help,h", "produce help message")
		("rasterize,r", "partition with rasterization")
		("qtree,q", "partition with qtree")
		("raster_only", "query with raster only")
		("convex_hull", "use convex hall for filtering")
		("mer", "use maximum enclosed rectangle")
		("mer_sample_round", "how many rounds of sampling needed for MER generating")
		("triangulate", "use triangulate")

		("source,s", po::value<string>(&global_ctx.source_path)->required(), "path to the source")
		("target,t", po::value<string>(&global_ctx.target_path), "path to the target")
		("valid_path", po::value<string>(&global_ctx.valid_path), "path to the file with valid polygons")
		("threads,n", po::value<int>(&global_ctx.num_threads), "number of threads")
		("vpr,v", po::value<int>(&global_ctx.vpr), "number of vertices per raster")
		("vpr_end", po::value<int>(&global_ctx.vpr_end), "number of vertices per raster")
		("big_threshold,b", po::value<int>(&global_ctx.big_threshold), "up threshold for complex polygon")
		("small_threshold", po::value<int>(&global_ctx.small_threshold), "low threshold for complex polygon")
		("sample_rate", po::value<float>(&global_ctx.sample_rate), "sample rate")
		("latency,l","collect the latency information")
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	if (vm.count("help")) {
		cout << desc << "\n";
		exit(0);
	}
	po::notify(vm);

	global_ctx.use_grid = vm.count("rasterize");
	global_ctx.use_qtree = vm.count("qtree");
	global_ctx.perform_refine = !vm.count("raster_only");
	global_ctx.collect_latency = vm.count("latency");
	global_ctx.use_convex_hull = vm.count("convex_hull");
	global_ctx.use_mer = vm.count("mer");
	global_ctx.use_triangulate = vm.count("triangulate");


	return global_ctx;
}
