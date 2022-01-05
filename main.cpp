#TODO
#Fix it

#include <SFML/Graphics.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <iostream>
#include <complex>
#include <thread>
#include <chrono>

class FRAME;

//const unsigned int number_of_threads = std::thread::hardware_concurrency();
const unsigned int number_of_threads = 4;
const bool one_frame_only = true;

template<typename T>
struct coordinates{
	T coordinate_x;
	T coordinate_y;
	coordinates(T cx = 0, T cy = 0){
		coordinate_x = cx;
		coordinate_y = cy;
	}
};

template <typename T>
struct xy_range{
	T range_x_min;
	T range_x_max;
	T range_y_min;
	T range_y_max;
	xy_range(const T& range_x_max = 0, const T& range_x_min = 0, const T& range_y_max = 0, const T& range_y_min = 0) {
		this->range_x_max=range_x_max;
		this->range_x_min=range_x_min;
		this->range_y_max=range_y_max;
		this->range_y_min=range_y_min;
	}
	xy_range& operator=(const xy_range&(T range_x_max, T range_x_min, T range_y_max, T range_y_min)){
		this->range_x_max;
		this->range_x_min;
		this->range_y_max;
		this->range_y_min;
	}
};

class FRAME{
public:
	sf::Image *frame_image;
	uint16_t resolution_width;
	uint16_t resolution_height;
	float width_height_ratio;
	float height_width_ratio;
	coordinates<uint16_t> *resolution;

	FRAME(uint16_t resolution_width, uint16_t resolution_height){
		frame_image = new sf::Image;
		frame_image->create(resolution_width, resolution_height);
		this->resolution_width=resolution_width;
		this->resolution_height=resolution_height;
		resolution = new coordinates<uint16_t>(resolution_width, resolution_height);
		width_height_ratio=float(resolution_width)/resolution_height;
		height_width_ratio=float(resolution_height)/resolution_width;
	}

	coordinates<uint16_t> convert_1D_to_2D_coordinates(uint32_t coordinate){
		return coordinates<uint16_t>(coordinate%resolution_width, coordinate/resolution_width);
	}

	void set_pixel_1d(uint16_t coords, sf::Color color){
		frame_image->setPixel(coords%resolution_width, coords/resolution_width, color);
	}
};

uint32_t *coordinates_in_spiral_order_1D(coordinates<uint16_t> size_of_frame){
	// will give back 1D coordinates for 2D grid. The array will be the size of grid size_of_frame cord_x * cord_y
	uint32_t size = (size_of_frame.coordinate_x<size_of_frame.coordinate_y)*size_of_frame.coordinate_y 
	+ (size_of_frame.coordinate_y<size_of_frame.coordinate_x)*size_of_frame.coordinate_x
	+ (size_of_frame.coordinate_y==size_of_frame.coordinate_x)*size_of_frame.coordinate_x;

	uint32_t *new_coordinates = new uint32_t[size_of_frame.coordinate_x*size_of_frame.coordinate_y];
	// set size to the biggest coordinate
	int x = 0; // current position; x
    int y = 0; // current position; y
    int d = 0; // current direction; 0=RIGHT, 1=DOWN, 2=LEFT, 3=UP
    int c = 0; // counter
    int s = 1; // chain size
    // starting point
    x = ((int)ceil(size/2.0))-1;
    y = ((int)ceil(size/2.0))-1;
    for (int k=1; k<=(size-1); k++){
        for (int j=0; j<(k<(size-1)?2:3); j++){
            for (int i=0; i<s; i++){
				// skip pixels beyond grid
				if((x<size_of_frame.coordinate_x && x>-1) && (y<size_of_frame.coordinate_y && y>-1)){
					// convert 2D coordinates to 1D coordinates, then pack them up
					new_coordinates[c] = y*size_of_frame.coordinate_x+x;
					c++;
				}
                switch (d){
                    case 0: y = y + 1; break;
                    case 1: x = x + 1; break;
                    case 2: y = y - 1; break;
                    case 3: x = x - 1; break;
                }
            }
            d = (d+1)%4;
        }
        s = s + 1;
    }
	return new_coordinates;
}

class mandelbrot_set{
public:
	xy_range<double> range;
	std::complex<double> z_component;
	mandelbrot_set(xy_range<double> range){
		this->range=range;
		z_component=std::complex<double>(0, 0);
	}
	coordinates<double> window2mandelbort_coordinates_2d(coordinates<uint16_t> window_coordinates, FRAME *primary_frame){
		coordinates<double> new_coordinates;
		new_coordinates.coordinate_x = (window_coordinates.coordinate_x * primary_frame->width_height_ratio * (range.range_x_max - range.range_x_min)) / primary_frame->resolution_width + range.range_x_min;
		new_coordinates.coordinate_y = (window_coordinates.coordinate_y * (range.range_y_max - range.range_y_min)) / primary_frame->resolution_height + range.range_y_min;
		//new_coordinates.coordinate_x = (window_coordinates.coordinate_x * (range.range_x_max - range.range_x_min)) / primary_frame->resolution_width + range.range_x_min;
		//new_coordinates.coordinate_y = (window_coordinates.coordinate_y * (range.range_y_max - range.range_y_min)) / primary_frame->resolution_height + range.range_y_min;
		return new_coordinates;
	}
	uint16_t core_loop(std::complex<double> c_component, uint16_t iterations_max, uint8_t z_component_max = 2){
		unsigned int mandelbrot_iterations = 0;
		for(;(abs(z_component) < z_component_max && mandelbrot_iterations < iterations_max); mandelbrot_iterations++){
			z_component = z_component * z_component + c_component;
		}
		z_component=std::complex<double>(0, 0);
		return mandelbrot_iterations;
	}
};

struct thread_data_pkg{
	uint8_t thread_id;
	uint32_t *frame_coordinates_to_work_on; // which coordinates are its to work on. An array of 1D coordinates
	uint32_t frame_coordinates_amount; // how many frame coordinates it works on
	uint16_t iterations_max; // iterations it should do
	FRAME* frame_to_work_on;
	mandelbrot_set* mandelbrot_main;
};

// what every thread does.
void mandelbrot_thread(thread_data_pkg *data_for_thread){
// iterate through every coordinate you have to work on.
	//while(1){
		for(uint32_t coordinate_iter = 0; coordinate_iter<data_for_thread->frame_coordinates_amount; coordinate_iter++){
			auto converted_coordinates = data_for_thread->frame_to_work_on->convert_1D_to_2D_coordinates(data_for_thread->frame_coordinates_to_work_on[coordinate_iter]);
			coordinates<uint16_t> window_cords_worked_on = converted_coordinates;
			coordinates<double> mandel_cords_worked_on = data_for_thread->mandelbrot_main->window2mandelbort_coordinates_2d(converted_coordinates, data_for_thread->frame_to_work_on);
			sf::Uint8 iterations = data_for_thread->mandelbrot_main->core_loop(std::complex<double>(mandel_cords_worked_on.coordinate_x, mandel_cords_worked_on.coordinate_y), data_for_thread->iterations_max);
			data_for_thread->frame_to_work_on->set_pixel_1d(data_for_thread->frame_coordinates_to_work_on[coordinate_iter], sf::Color(iterations, 0, 0));
		}
	//}
}

// Get threads ready, and dispatch them
void mandelbrot_control(FRAME *primary_frame){
	mandelbrot_set mandelbrot_main(xy_range<double>(2, -2, 2, -2));
	uint32_t *frame_coordinates_in_spiral = coordinates_in_spiral_order_1D(*primary_frame->resolution);
	thread_data_pkg *data_for_thread = new thread_data_pkg[number_of_threads];
	uint32_t num_coords_for_thread = primary_frame->resolution_width*primary_frame->resolution_height/number_of_threads;

	std::thread* working_threads = new std::thread[number_of_threads];
	for(uint8_t threads_iter = 0; threads_iter<number_of_threads; threads_iter++){
		// assign coordinates to a thread. So every next thread handles every next coordinate
		data_for_thread[threads_iter].frame_coordinates_to_work_on=new uint32_t[num_coords_for_thread];
		uint32_t array_iter = 0;
		for(uint32_t absurd = threads_iter; absurd<primary_frame->resolution_width*primary_frame->resolution_height; absurd+=number_of_threads){
			data_for_thread[threads_iter].frame_coordinates_to_work_on[array_iter] = frame_coordinates_in_spiral[absurd];
			array_iter++;
		}
		// all the other information necessary for the thread to work on
		data_for_thread[threads_iter].frame_coordinates_amount=num_coords_for_thread;
		data_for_thread[threads_iter].iterations_max=250;
		data_for_thread[threads_iter].frame_to_work_on=primary_frame;
		data_for_thread[threads_iter].thread_id = threads_iter;
		data_for_thread[threads_iter].mandelbrot_main = &mandelbrot_main;
		// dispatch the thread, and detach so it always works in the background
		working_threads[threads_iter] = std::thread(mandelbrot_thread, (&data_for_thread[threads_iter]));
//		mandelbrotter.detach();
		//working_threads[threads_iter].join();
		//mandelbrot_thread(data_for_thread[threads_iter]);
	}
	if(one_frame_only){
		for(uint8_t threads_iter = 0; threads_iter<number_of_threads; threads_iter++){
			working_threads[threads_iter].join();
		}
	}
}

uint32_t fps_to_frametime(uint16_t fps){
	return 1000/fps;
}

int main(){
	FRAME* primary_frame;
	uint16_t resolution_width, resolution_height;
	std::cout << "Set resolution:\nWidth: ";
	std::cin >> resolution_width;
	std::cout << "Height: ";
	std::cin >> resolution_height;
	primary_frame = new FRAME(resolution_width, resolution_height);
	mandelbrot_control(primary_frame);
	//std::this_thread::sleep_for(std::chrono::milliseconds(300));
	sf::Texture finished_frame_texture;
	finished_frame_texture.create(primary_frame->resolution_width, primary_frame->resolution_height);
	sf::Sprite finished_frame_sprite;
	sf::RenderWindow window(sf::VideoMode(primary_frame->resolution_width, primary_frame->resolution_height), "Mandelbrotter");

	uint16_t fps_limit = 24;
	sf::Clock deltaClock; // time needed to output a frame
	sf::Time frametime = sf::milliseconds(fps_to_frametime(fps_limit));
	while (window.isOpen()){
		sf::Event event;
		while (window.pollEvent(event)){
			if (event.type == sf::Event::Closed)
				window.close();
		}

		finished_frame_texture.loadFromImage(*primary_frame->frame_image);
		finished_frame_sprite.setTexture(finished_frame_texture);

		window.clear();
		window.draw(finished_frame_sprite);
		sf::Time delta = deltaClock.restart();
		std::this_thread::sleep_for(std::chrono::milliseconds(frametime.asMilliseconds()-delta.asMilliseconds())); // wait the exact amount of time for next frame
		window.display();
	}
	return 0;
}
