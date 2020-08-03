
#include "VCDFileParser.hpp"
#include "cxxopts.hpp"
#include "gitversion.h"

void print_scope_signals(VCDFile * trace, VCDScope * scope, std::string local_parent)
{
	int end_time=trace->get_timestamps()->back();
    for(VCDSignal * signal : scope -> signals) {
        std::cout << signal -> hash << "\t" << trace->get_signal_values(signal -> hash)->size() << "\t"
                    << local_parent << "." << signal -> reference;

        if(signal -> size > 1) {
            std::cout << "[" << signal -> lindex << ":" << signal -> rindex << "]";
        } else if (signal -> lindex >= 0) {
            std::cout << "[" << signal -> lindex << "]";
        }
        
        std::cout << std::endl;
	int sp=0;
	int last_nz=0;
	int last_z=0;
	int delta;
	int tr=0;
	int current=-1;
	VCDSignalValues * signal_values = trace->get_signal_values(signal -> hash); 
	for (VCDTimedValue * v : *signal_values) {
        //	std::cout << "Time: " << value->time << "\tValue : " << value->value->get_value_bit()  << " (" << value->value->get_value_vector() << ")" << std::endl;
		if (v->value->get_value_bit()) 
		{
		  //std::cout << "non Zero" << std::endl;
		  last_nz=v->time;
	  	} else 
		{
		  last_z=v->time;
		  sp += last_z - last_nz;
		  //std::cout << "sp:" << sp << std::endl;
		}
		if (v->value->get_value_bit() != current){
		  current=v->value->get_value_bit();
		  tr+=1;
		}
	}
	if (signal_values->back()->value->get_value_bit()) 
	{
	  //std::cout << "non Zero " << sp << " " << end_time << " -  " << last_nz <<  std::endl;
	  //last_nz=value->time;
	  sp += end_time - last_nz;
  	} 
	std::cout << "SP: " << (float)sp/end_time << " TR:" << tr << std::endl;	
    }
}

void print_activity(VCDFile * trace, VCDScope * scope, std::string local_parent)
{
	int end_time=trace->get_timestamps()->back();
	int sp;
	int last_nz;
	int last_z;
	int delta;
	int tr;
	int current=-1;
	int b;
	int value;
	int size;
	VCDSignalValues * signal_values;
	// For each signal

    for(VCDSignal * signal : scope -> signals) {
	// What to do if vector, try dumbest option, just for on each bit
	for ( b=0; b < signal->size; b++) {
		tr=0;
		sp=0;
		last_z=0;
		last_nz=0;
		current=-1;
        	std::cout <<  local_parent << "." << signal -> reference;
        	if(signal -> size > 1) {
        	    std::cout << "[" << b << "]";
        	} else if (signal -> lindex >= 0) {
        	    //std::cout << "[" << signal -> lindex << "]";
        	}
		//std::cout << std::endl;
		signal_values = trace->get_signal_values(signal -> hash); 
		for (VCDTimedValue * v : *signal_values) {
			if (v->value->get_type() == VCD_VECTOR) 
			{
			  	size = v->value->get_value_vector()->size();
				if(b < size) 
				{
					value = v->value->get_value_vector()->at(size-b-1);
					//std::cout << "at(b) ";
					//for(VCDBit bit: *v->value->get_value_vector()) std::cout << bit;
				} else {
					value = ( v->value->get_value_vector()->at(size-1) == VCD_X ) ? VCD_X :  VCD_0 ;
					//std::cout << "at(size-1) ";
					//for(VCDBit bit: *v->value->get_value_vector()) std::cout << bit;
				}
			} else
			{
				value=v->value->get_value_bit();
				//std::cout << "bit ";
			}	
			if (value == VCD_1 ) 
			{
			  last_nz=v->time;
		  	} else if (value == VCD_0)
			{
			  last_z=v->time;
			  if (current == VCD_1) sp += last_z - last_nz;
			}
			// printf(" 0x%x 0x%x \n", current, value);
			if ( value != current ){
			  if ( (current == VCD_0) || (current ==VCD_1))
			  	tr+=1;
			  current=value;
			}
		}
		// Get final value
		if (signal_values->back()->value->get_type() == VCD_VECTOR) 
		{
			if(b < signal_values->back()->value->get_value_vector()->size())
				value = signal_values->back()->value->get_value_vector()->at(b);
			else
				value = signal_values->back()->value->get_value_vector()->at(signal_values->back()->value->get_value_vector()->size()-1);
		} else
			value=signal_values->back()->value->get_value_bit();
		if ( value == VCD_1) 
		{
		  //std::cout << "end - lastZero " << sp << " " << end_time << " -  " << last_z <<  std::endl;
		  //last_nz=value->time;
		  sp += end_time - last_z;
		} else if (value == VCD_0) 
		{
		  //sp += end_time - last_nz;
		} 
		//std::cout << "," << sp << "," << end_time  << "," << (float)sp/end_time << "," << tr << std::endl;	
		std::cout << "," << (float)sp/end_time << "," << (float)tr << std::endl;	
	}
    }
}

void traverse_scope(std::string parent, VCDFile * trace, VCDScope * scope, bool instances, bool fullpath)
{
    std::string local_parent = parent;

    if (parent.length())
        local_parent += ".";
    local_parent += scope->name;
    if (instances)
        std::cout << "Scope: " << local_parent  << std::endl;
    if (fullpath)
        //print_scope_signals(trace, scope, local_parent);
        print_activity(trace, scope, local_parent);
    for (auto child : scope->children)
        traverse_scope(local_parent, trace, child, instances, fullpath);
}
/*!
@brief Standalone test function to allow testing of the VCD file parser.
*/
int main (int argc, char** argv)
{

    cxxopts::Options options("vcdtoggle", "Count number of signal toggles in VCD file");
    options.add_options()
        ("h,help", "show help message")
        ("v,version", "Show version")
        ("i,instances", "Show only instances")
        ("r,header", "Show header")
        ("u,fullpath", "Show full signal path")
        ("s,start", "Start time (default to 0)", cxxopts::value<VCDTime>())
        ("e,end", "End time (default to end of file)", cxxopts::value<VCDTime>())
        ("f,file", "filename containing scopes and signal name regex", cxxopts::value<std::string>())
        ("positional", "Positional pareameters", cxxopts::value<std::vector<std::string>>())
    ;
    options.parse_positional({"positional"});
    auto result = options.parse(argc, argv);

    if (result["help"].as<bool>())
        std::cout << options.help() << std::endl;
    if (result["version"].as<bool>())
        std::cout << "Version:  0.1\nGit HEAD: "  << gitversion << std::endl;

    std::string infile (result["positional"].as<std::vector<std::string>>().back());

    VCDFileParser parser;

    if (result.count("start"))
        parser.start_time = result["start"].as<VCDTime>();

    if (result.count("end"))
        parser.end_time = result["end"].as<VCDTime>();

    VCDFile * trace = parser.parse_file(infile);
    bool instances = result["instances"].as<bool>();
    bool fullpath = result["fullpath"].as<bool>();

    if (trace) {
        if (result["header"].as<bool>()) {
            std::cout << "Version:       " << trace -> version << std::endl;
            std::cout << "Comment:       " << trace -> comment << std::endl;
            std::cout << "Date:          " << trace -> date << std::endl;
            std::cout << "Signal count:  " << trace -> get_signals() -> size() <<std::endl;
            std::cout << "Sim Time:" << trace -> get_timestamps() -> back() << std::endl;
            std::cout << "Times Recorded:" << trace -> get_timestamps() -> size() << std::endl;
            if (fullpath)
                std::cout << "Hash\tToggles\tFull signal path\n";
        }    
        // Print out every signal in every scope.
		std::cout << "signal,static_probability,toggle" << std::endl;
        	traverse_scope(std::string(""), trace, trace->root_scope, instances, fullpath);

        delete trace;
        
        return 0;
    } else {
        std::cout << "Parse Failed." << std::endl;
        return 1;
    }
}
