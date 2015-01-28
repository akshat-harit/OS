#include "rvm.h"
using namespace std;

std::vector<segment_node> segment_list; 

static char* seg_file_ext = ".seg";
static char* log_file_ext = ".log";
static trans_t global_tid = 0;
static char* LOG_FILE;
static rvm_t* rvm;
static int verbose = 1;
#define LOG_LIMIT 1024 //5*1024*1024  //100

int file_dir_exists(const char* name){
    struct stat buffer;
    return (stat(name, &buffer) == 0);
}

// If file or directory doesn't exist, creates it, else returns without doing anything.
int create_file_dir(const char* name, char* type) {
    if(!file_dir_exists(name)){
        if(mkdir(name, S_IRWXU | S_IRWXG)){
            verbose==0 ? : printf("Error while creating %s \'%s\'\n", type, name);
            return 0;
        } else {
            verbose==0 ? : printf("%s \'%s\' created successfully!\n", type, name);
        }
    } else {
        verbose==0 ? : printf("%s \'%s\' already exists.\n", type, name);
    }
    return 1;
}

// Initialize the library with the specified directory as backing store.
rvm_t rvm_init(const char* directory) {
    verbose==0 ? : printf("\n=== RVM INIT ===\nInitialize the library with the directory %s as backing store.\n", directory);

    rvm = new rvm_t;
    if (!create_file_dir(directory, "Directory")){
        return *rvm;
    }

    // Inititalize the RVM directory and log file
    rvm->directory = (char*) malloc(strlen(directory) + 1);
    strcpy(rvm->directory, directory);

    rvm->logfile = (char*) malloc(strlen(directory) + strlen(log_file_ext) + 1);
    sprintf(rvm->logfile, "%s/rvm%s", directory, log_file_ext);
    LOG_FILE=(char *)malloc(strlen(rvm->logfile)+1);
    strcpy(LOG_FILE,rvm->logfile);
    return *rvm;
}

/*Delete the segment file if it exists and is not mapped*/
void rvm_destroy(rvm_t rvm, const char* seg_name){
    verbose==0 ? : printf("Deleting the RVM segment - %s\n", seg_name);

    // Verify that the segment is not currently mapped.
    for(vector<segment_node>::iterator head=segment_list.begin(); head!=segment_list.end(); head++){
        if(!strcmp(head->segment->name, seg_name)){
            printf("Segment is currently mapped. Will not delete it.");
            return;
        } 
    }

    // The segment is not currently mapped, so erase its backing store.
    char* back_store = get_seg_file_path(seg_name);
    if(file_dir_exists(back_store)){
        if(remove(back_store)){
            perror("Failed: Removing file*****: ");
            verbose==0 ? : printf("Error deleting segment: %s\n", back_store);
        }
    }
}

/*Map the segment from disk to memory, if it doesnt exist on disk create it*/
void* rvm_map(rvm_t rvm, const char* seg_name, int size_to_create){
    verbose==0 ? : printf("\nRequest for mapping of segment '%s'\n", seg_name);
    int is_new_segment = 0;
    
    for(vector<segment_node>::iterator head=segment_list.begin(); head!=segment_list.end(); head++){
        if(!strcmp(head->segment->name, seg_name)){
            verbose ==0 ? :printf("Segment is currently mapped. Will not remap it\n");
            //segment_node &seg_obj = *(head->segment);
            return head->segment->data;
        } 
    }

    //Load backing store into memory
    char * proper_segfile_name = get_seg_file_path(seg_name);

    if( access( proper_segfile_name, F_OK ) == -1 ) {
    	verbose==0 ? : printf("Segment '%s' doesn't exist on disk. Creating it.\n", seg_name);
	    is_new_segment = 1;
	}

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int seg_file = open(proper_segfile_name, O_RDWR | O_CREAT, 0666);
    if(seg_file== -1){
        perror("File creation failed"); 
        }
    struct stat sb;

    if (stat(proper_segfile_name, &sb) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
    }
    if(sb.st_size<size_to_create)
        ftruncate (seg_file, off_t(size_to_create));

    close(seg_file);
    
    

    //Create segment node to store data
    segment_t* node = new segment_t;
    node->name = (char*) malloc(sizeof( strlen(seg_name) +  1));
    strcpy(node->name, seg_name);
    node->data = (void*) malloc(sizeof(char) * size_to_create);
    node->size = size_to_create;

    if(!is_new_segment){
    	//Load data from backing store
	    // rvm_truncate_log(rvm);
	    proper_segfile_name = get_seg_file_path(seg_name);
	    seg_file=open(proper_segfile_name, O_RDWR, 0666);
	    if(seg_file== -1){
	        perror("File read after truncate log failed ");
	        printf("File name: %s\n", proper_segfile_name) ;
	    }
	    void* memory=mmap(NULL,
	                    size_to_create,
	                    PROT_READ | PROT_WRITE, 
	                    MAP_SHARED, 
	                    seg_file, 
	                    0);
	    if(memory==MAP_FAILED){
	        printf("%s file mapping failed\n", proper_segfile_name);
	        perror("Mapping Failed\n");
	        exit(2);

	    }

	    verbose==0 ?:std::cout<<"Backing segment for '"<< seg_name <<"' found. Restoring data.\n";
	    memcpy(node->data, memory, size_to_create);
	    close(seg_file);
	}
    // Create segment and add it to the list of segments
    segment_node* new_seg = new segment_node;
    new_seg->segment  = node;
    new_seg->txn      = -1;

    //Don't replay log file entries if this segment has been freshly initialized with new backing store
    if (!is_new_segment) {
    	read_seg_log(seg_name, new_seg->segment);
    }

    segment_list.push_back(*new_seg);
    delete new_seg;
    return node->data;
}

char* get_seg_file_path(const char* seg_name){
    char *result = (char*) malloc( strlen(rvm->directory) + strlen(seg_name) + strlen(seg_file_ext) + 1);
    sprintf(result, "%s/%s%s", rvm->directory, seg_name, seg_file_ext);
    return result;
}

trans_t rvm_begin_trans(rvm_t rvm, int num_segs, void** seg_bases){
    if(verbose==1)
    	std::cout<<"\nTransaction started on "<<num_segs<<" segments"<<endl;
    int initial_segment = 0;
    
    //Check for same transaction
    for(initial_segment =0; initial_segment  < num_segs; initial_segment++){ 
        for(vector<segment_node>::iterator i=segment_list.begin(); i!=segment_list.end(); i++) 
            {if(i->segment->data == seg_bases[initial_segment]) 
                if(i->txn != -1) return (trans_t)-1; 
        } 
    }
    trans_t tid = global_tid++;
    /* set transaction ids for the segments that are to be modified */
    vector<segment_node>::iterator seg;
    for(initial_segment=0; initial_segment<num_segs; ++initial_segment){
        for(seg=segment_list.begin(); seg!=segment_list.end(); seg++){           
            if(seg->segment->data == seg_bases[initial_segment]) {
                seg->txn = tid;
                verbose==0?:std::cout<<"Setting transaction id "<<tid<<" for segment '"<<seg->segment->name<<"'\n";
            }
        }
    }
    return tid;
}

void rvm_about_to_modify(trans_t tid, void* seg_base, int offset, int size){
    for(vector<segment_node>::iterator seg=segment_list.begin(); seg!=segment_list.end(); seg++){
        if(seg->txn == tid && seg->segment->data == seg_base){
            offset_obj* new_offset = new offset_obj;
            new_offset -> val = offset;
            new_offset -> size  = size;
            new_offset -> old_data =malloc(size);
            memcpy(new_offset->old_data,(seg->segment->data+offset), size);
            seg->offset_list.push_back(*new_offset);
            delete new_offset;
            verbose==0 ?: std::cout<<"Modifying segment '"<<( seg->segment->name)<<"'' at offset "<<offset<<std::endl;
            break;
        }
      }
}

void rvm_commit_trans(trans_t tid){
	if(verbose==1)
	std::cout<<"\nCommitting transaction id "<< tid<<std::endl;
    FILE* log_file;
    log_file = fopen(LOG_FILE, "a");
    struct stat sb;
    if (stat(LOG_FILE, &sb) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
    }
    if(sb.st_size>LOG_LIMIT){
        rvm_truncate_log(*rvm);
    }


    if(log_file == NULL)
        std::cerr<<"Failed to open the log file\n";
    if(verbose==1)
    	std::cout<<"\nWriting to the log file for transaction id: "<< tid<<std::endl;
	
	for(vector<segment_node>::iterator seg=segment_list.begin(); seg!=segment_list.end(); seg++){  
        if(seg->txn == tid){
            segment_node &seg_obj = *seg;
            save_seg_file(seg_obj, log_file);
            seg->offset_list.clear();
        }
    }


    remove_seg_from_transaction(tid);
    fflush(log_file);
}

void rvm_abort_trans(trans_t tid) {
    verbose==0?:std::cout<<"Abort transaction "<<tid<<std::endl;

    vector<segment_node>::iterator seg;
	for(seg=segment_list.begin(); seg!=segment_list.end(); seg++){
        if (seg->txn == tid) {
            seg->txn=-1;
            for(vector<offset_obj>::iterator off=seg->offset_list.begin(); off!=seg->offset_list.end(); off++){
                memcpy(seg->segment->data+off->val, off->old_data, off->size);
            }
            seg->offset_list.clear();       
        }
    }

}

int save_seg_file(segment_node seg, FILE* file){
    verbose==0 ? : printf("Writing the segment '%s' to the disk \n", seg.segment->name);
    string SEPERATOR="\n";
    string START="START";
    string data("");
    vector<offset_obj> base_offset=seg.offset_list;

    for(vector<offset_obj>::iterator off=seg.offset_list.begin(); off!=seg.offset_list.end(); off++){
        data=START+SEPERATOR+seg.segment->name;
        data += SEPERATOR + to_string(off->val)
                +SEPERATOR + to_string(off->size)+SEPERATOR;
        fwrite(data.c_str(), data.length(), 1, file) ;       
        fwrite((char *)seg.segment->data+off->val, off->size, 1, file) ;
        data=SEPERATOR+"END"+SEPERATOR;
        fwrite(data.c_str(), data.length(), 1, file) ;
    }
    
    verbose==0? : std::cout<<"Adding log entry: \n"; //??<<data<<std::endl;
    return 1;
}

int str_compare(const char* a, const char* b){
    string a_str(a);
    string b_str(b);
    a_str.erase(a_str.find_last_not_of("\n\r\t")+1);
    return a_str==b_str;
}

void read_seg_log(const char* seg_name, segment_t* seg){
    verbose==0?: std::cout<<"\nApplying disk log changes to in-memory segment '"<<seg_name<<"'\n";

    FILE* log_file;
    if(file_dir_exists(LOG_FILE)) {
        log_file = fopen(LOG_FILE, "r");
        if(log_file == NULL) {
            std::cerr<<"Failed to open segment file "<<LOG_FILE<<std::endl;
            return ;
        }
        verbose==0? : std::cout<< "Opened log file.\n";
        char line[LINE_MAX];
        char size[LINE_MAX];
        char offset_value[LINE_MAX];
        char data_char[LINE_MAX];
        char current_seg_name[LINE_MAX];
        while(fgets(line, LINE_MAX, log_file) != NULL){
            string start(line);
            string data("");
            if(start=="START\n"){
                fgets(current_seg_name, LINE_MAX, log_file);
                if(!str_compare(current_seg_name, seg_name)){
                    continue;
                }
                fgets(offset_value, LINE_MAX, log_file);
                fgets(size, LINE_MAX, log_file);
                fread(seg->data+atoi(offset_value),atoi(size),1, log_file);
                fseek(log_file, 1, SEEK_CUR);
                //data.erase(data.find_last_not_of("\n\r\t")+1);
                //verbose==0 ?: std::cout<<"Read from log.\n";
                //strcpy((char*)seg->data + atoi(offset_value), data.c_str());
            }
        }
        verbose==0 ?: std::cout<<"Done applying log changes to memory.\n";
    }
}

void remove_seg_from_transaction(trans_t tid){
	verbose==0?:std::cout<<"Commit to log done for transaction id "<<tid<<". Removing segments from memory."<<std::endl;
    
    vector<segment_node>::iterator seg;
    for(seg=segment_list.begin(); seg!=segment_list.end(); seg++){
        if(seg->txn == tid){
            seg->txn = -1;
        }
    }
}

void rvm_unmap(rvm_t rvm, void* seg_base){
    vector<segment_node>::iterator seg;
    for(seg=segment_list.begin(); seg!=segment_list.end(); seg++){
        if(seg->segment->data == seg_base){
            break;
        }
    }    
    verbose == 0 ?: std::cout<<"Unmapped Segment: "<<seg->segment->name<<std::endl;
    segment_list.erase(seg);

}

void rvm_truncate_log(rvm_t rvm){
    verbose==0 ? : std::cout<<"**Truncating Log File**"<<LOG_FILE<<std::endl;
    FILE* log_file;
    if(file_dir_exists(LOG_FILE)) {
        log_file = fopen(LOG_FILE, "r");
        if(log_file == NULL) {
            std::cerr<<"Failed to open log file "<<LOG_FILE<<std::endl;
            return ;
        }

        verbose==0? : std::cout<< "Opened log file.\n";
        char line[LINE_MAX];
        char size[LINE_MAX];
        char offset_value[LINE_MAX];
        char data_char[LINE_MAX];
        char current_seg_name[LINE_MAX];
        char * c;
        while(fgets(line, LINE_MAX, log_file) != NULL){
            string start(line);
            string data("");
            if(start=="START\n"){
            	//Get the segment name from the log entry and compute backing file name
                fgets(current_seg_name, LINE_MAX, log_file);
                string temp(current_seg_name);
                temp.erase(temp.find_last_not_of("\n\r\t")+1);
                char* proper_segfile_name=(char *)malloc(temp.length()+1);
                strcpy(proper_segfile_name, temp.c_str());
                proper_segfile_name = get_seg_file_path(proper_segfile_name);

                if( access( proper_segfile_name, F_OK ) == -1 ) {
			    	verbose==0 ? : printf("\nSegment '%s' doesn't exist on disk. Skipping log entry.\n", current_seg_name);
			    	continue;
				}

				// Open the segment file
                mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                int seg_file=open (proper_segfile_name, O_RDWR, 0666);
                // printf("File name: %s\n", proper_segfile_name);
                struct stat sb;
                if(seg_file== -1){
                    perror("File open failed");
                }
                if (stat(proper_segfile_name, &sb) == -1) {
                        perror("stat");
                        exit(EXIT_FAILURE);
                }
                // printf("Mapping memory using mmap\n");
                void* memory=mmap(NULL,
                    sb.st_size, 
                    PROT_READ | PROT_WRITE, 
                    MAP_SHARED, 
                    seg_file, 
                    0);
                if(memory==MAP_FAILED){
                    printf("%s file mapping failed\n", proper_segfile_name);
                    perror("Mapping Failed\n");
                    exit(2);
                }

                //Get offset and data and write it to the mapped memory segment
                fgets(offset_value, LINE_MAX, log_file);
                fgets(size, LINE_MAX, log_file);
                c = (char *)malloc(atoi(size));
                fread(c ,atoi(size),1, log_file);
                fseek(log_file, 1, SEEK_CUR);
                fgets(data_char, LINE_MAX, log_file);
                string temp_data(data_char);
                if(temp_data!="END\n"){
                    verbose==0?: std::cerr<<"Error in log file reading\n";
                    exit(2);
                }
                memcpy(memory + atoi(offset_value), c, atoi(size));                
                verbose==0?: std::cout<<"Read data from log.\n";
                verbose==0?: std::cout<<"Offset value: "<<atoi(offset_value);//<<"\nData\n"<<data.c_str();
                verbose==0?: std::cout<<"\nSize: "<<atoi(size)<<std::endl;
                free(c);
                // free(proper_segfile_name);
                int file_location=ftell(log_file);
                close(seg_file);
                if(file_location==SEEK_END){
                    printf("A file error has occurred. %d\n");
                    break;
                }
            }

        }
    }
    freopen(LOG_FILE, "w", log_file);
    fclose(log_file);
}

void rvm_verbose(int enable_flag){
    verbose = enable_flag;
}