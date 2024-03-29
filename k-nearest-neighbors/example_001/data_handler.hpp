//
//  MIT License
//  
//  Copyright (c) 2019 Miguel Angel Moreno
//  Based on original code by Gerard Taylor
//  
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//  

#ifndef __DATA_HANDLER_H
#define __DATA_HANDLER_H

#include <vector>
#include <string>
#include <map>
#include <unordered_set>
#include <math.h>
#include "fstream"
#include "stdint.h"
#include "data.hpp"


using namespace std;


class DataHandler
{

public:

    const double TRAIN_SET_PERCENT = .1;
    const double TEST_SET_PERCENT = .075;
    const double VALID_SET_PERCENT = 0.005;

    DataHandler() {

        data_array = new vector<data *>;
        training_data = new vector<data *>;
        test_data = new vector<data *>;
        validation_data = new vector<data *>;
    }

    ~DataHandler() { 
    
    }

    
    void read_csv(string path, string delimiter)
    {
        class_counts = 0;
        ifstream data_file(path.c_str());
        string line;
        while(getline(data_file, line))
        {
            if(line.length() == 0) continue;
            data *d = new data();
            d->set_normalized_feature_vector(new vector<double>());
            size_t position = 0;
            string token;
            while((position = line.find(delimiter)) != string::npos)
            {
                token = line.substr(0, position);
                d->append_to_feature_vector(stod(token));
                line.erase(0, position + delimiter.length());
            }
            if(classMap.find(line) != classMap.end())
            {
                d->set_label(classMap[line]);
            } else 
            {
                classMap[line] = class_counts;
                d->set_label(classMap[token]);
                class_counts++;
            }
            data_array->push_back(d);
        }
        feature_vector_size = data_array->at(0)->get_normalized_feature_vector()->size();
    }

    void read_input_data(string path)
    {
        uint32_t magic = 0;
        uint32_t num_images = 0;
        uint32_t num_rows = 0;
        uint32_t num_cols = 0;

        unsigned char bytes[4];
        FILE *f = fopen(path.c_str(), "r");
        if(f)
        {
            int i = 0;
            while(i < 4)
            {
                if(fread(bytes, sizeof(bytes), 1, f))
                {
                    switch(i)
                    {
                        case 0:
                            magic = fix_endianness(bytes);
                            i++;
                            break;
                        case 1:
                            num_images = fix_endianness(bytes);
                            i++;
                            break;
                        case 2:
                            num_rows = fix_endianness(bytes);
                            i++;
                            break;
                        case 3:
                            num_cols = fix_endianness(bytes);
                            i++;
                            break;
                    }
                }
            }
            printf("Done getting file header.\n");
            uint32_t image_size = num_rows * num_cols;
            for(i = 0; i < num_images; i++)
            {
                data *d = new data();
                d->set_feature_vector(new vector<uint8_t>());
                uint8_t element[1];
                for(int j = 0; j < image_size; j++)
                {
                    if(fread(element, sizeof(element), 1, f))
                    {
                        d->append_to_feature_vector(element[0]);
                    }
                }
                data_array->push_back(d);
            }
            normalize();
            feature_vector_size = data_array->at(0)->get_feature_vector()->size();
            printf("Successfully read %lu data entries.\n", data_array->size());
            printf("The Feature Vector Size is: %d\n", feature_vector_size);
        } else
        {
            printf("Invalid Input File Path\n");
            exit(1);
        }
    }

    void read_label_data(string path)
    {
        uint32_t magic = 0;
        uint32_t num_images = 0;
        unsigned char bytes[4];
        FILE *f = fopen(path.c_str(), "r");
        if(f)
        {
            int i = 0;
            while(i < 2)
            {
                if(fread(bytes, sizeof(bytes), 1, f))
                {
                    switch(i)
                    {
                        case 0:
                            magic = fix_endianness(bytes);
                            i++;
                            break;
                        case 1:
                            num_images = fix_endianness(bytes);
                            i++;
                            break;
                    }
                }
            }

            for(unsigned j = 0; j < num_images; j++)
            {
                uint8_t element[1];
                if(fread(element, sizeof(element), 1, f))
                {
                    data_array->at(j)->set_label(element[0]);
                }
            }

            printf("Done getting Label header.\n");
        } 
        else
        {
            printf("Invalid Label File Path\n");
            exit(1);
        }
    }

    void split_data()
    {
        unordered_set<int> used_indexes;
        int train_size = data_array->size() * TRAIN_SET_PERCENT;
        int test_size = data_array->size() * TEST_SET_PERCENT;
        int valid_size = data_array->size() * VALID_SET_PERCENT;

        // Training Data

        int count = 0;
        while(count < train_size)
        {
            int index = rand() % data_array->size(); // 0 and data_array->size() - 1
            if(used_indexes.find(index) == used_indexes.end())
            {
                training_data->push_back(data_array->at(index));
                used_indexes.insert(index);
                count++;
            }
        }

        // Test Data

        count = 0;
        while(count < test_size)
        {
            int index = rand() % data_array->size(); // 0 and data_array->size() - 1
            if(used_indexes.find(index) == used_indexes.end())
            {
                test_data->push_back(data_array->at(index));
                used_indexes.insert(index);
                count++;
            }
        }

        // Test Data

        count = 0;
        while(count < valid_size)
        {
            int index = rand() % data_array->size(); // 0 and data_array->size() - 1
            if(used_indexes.find(index) == used_indexes.end())
            {
                validation_data->push_back(data_array->at(index));
                used_indexes.insert(index);
                count++;
            }
        }

        printf("Training Data Size: %lu.\n", training_data->size());
        printf("Test Data Size: %lu.\n", test_data->size());
        printf("Validation Data Size: %lu.\n", validation_data->size());
    }

    void count_classes()
    {
        int count = 0;
        for(unsigned i = 0; i < data_array->size(); i++)
        {
            if(class_map.find(data_array->at(i)->get_label()) == class_map.end())
            {
                class_map[data_array->at(i)->get_label()] = count;
                data_array->at(i)->set_enumerated_label(count);
                count++;
            }
            else 
            {
                data_array->at(i)->set_enumerated_label(class_map[data_array->at(i)->get_label()]);
            }
        }
        class_counts = count;
        for(data *data : *data_array)
            data->setClassVector(class_counts);
        printf("Successfully Extraced %d Unique Classes.\n", class_counts);
    }

    void normalize()
    {
        vector<double> min_list, max_list;
        // fill min and max lists
        
        data *d = data_array->at(0);
        for(auto val : *d->get_feature_vector())
        {
            min_list.push_back((double) val);
            max_list.push_back((double) val);
        }

        for(int i = 1; i < data_array->size(); i++)
        {
            d = data_array->at(i);
            for(int j = 0; j < d->get_feature_vector_size(); j++)
            {
                if(d->get_feature_vector()->at(j) < min_list.at(j))
                    min_list[j] = (double)d->get_feature_vector()->at(j);
                if(d->get_feature_vector()->at(j) > max_list.at(j))
                    max_list[j] = (double)d->get_feature_vector()->at(j);
            }
        }

        // normalize data array
        
        for(int i = 0; i < data_array->size(); i++)
        {
            data_array->at(i)->set_normalized_feature_vector(new vector<double>());
            data_array->at(i)->setClassVector(class_counts);
            for(int j = 0; j < data_array->at(i)->get_feature_vector_size(); j++)
            {
                double val_prime = data_array->at(i)->get_feature_vector()->at(j) - min_list.at(j);
                val_prime /= (max_list[j] - min_list[j]);
                if(isnan(val_prime))
                    val_prime = 0;
                data_array->at(i)->append_to_feature_vector(val_prime);
            }
        }
    }

    void print()
    {
        printf("Training Data:\n");
        for(auto data : *training_data)
        {
            for(auto value : *data->get_normalized_feature_vector())
            {
                printf("%.3f,", value);
            }
            printf(" ->     %d\n", data->get_label());
        }

        printf("Test Data:\n");
        for(auto data : *test_data)
        {
            for(auto value : *data->get_normalized_feature_vector())
            {
                printf("%.3f,", value);
            }
            printf(" ->     %d\n", data->get_label());
        }

        printf("Validation Data:\n");
        for(auto data : *validation_data)
        {
            for(auto value : *data->get_normalized_feature_vector())
            {
                printf("%.3f,", value);
            }
            printf(" ->     %d\n", data->get_label());
        }

    }
    
    int get_class_counts()
    {
        return class_counts;
    }

    int get_data_array_size()
    {
        return data_array->size();
    }

    int get_training_data_size()
    {
        return training_data->size();
    }

    int get_test_data_size()
    {
        return test_data->size();
    }
    
    int get_validation_size()
    {
        return validation_data->size();
    }

    uint32_t fix_endianness(const unsigned char* bytes)
    {
        return (uint32_t)((bytes[0] << 24) |
                                            (bytes[1] << 16)    |
                                            (bytes[2] << 8)     |
                                            (bytes[3]));
    }

    vector<data *> * get_training_data()
    {
        return training_data;
    }

    vector<data *> * get_test_data()
    {
        return test_data;
    }

    vector<data *> * get_validation_data()
    {
        return validation_data;
    }

    map<uint8_t, int> get_class_map()
    {
        return class_map;
    }


private:

    vector<data *> *data_array; // all of the data
    vector<data *> *training_data;
    vector<data *> *test_data;
    vector<data *> *validation_data;
    
    int class_counts;
    int feature_vector_size;

    map<uint8_t, int> class_map;
    map<string, int> classMap; //string key

};

#endif

