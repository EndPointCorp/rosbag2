// Copyright 2020 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "rosbag2_compression/sequential_compression_reader.hpp"
#include "rosbag2_cpp/converter_options.hpp"
#include "rosbag2_cpp/plugins/plugin_utils.hpp"
#include "rosbag2_cpp/readers/sequential_reader.hpp"
#include "rosbag2_cpp/reader.hpp"
#include "rosbag2_storage/storage_interfaces/read_only_interface.hpp"
#include "rosbag2_storage/storage_interfaces/read_write_interface.hpp"
#include "rosbag2_storage/storage_filter.hpp"
#include "rosbag2_storage/storage_options.hpp"
#include "rosbag2_storage/topic_metadata.hpp"

#include "./pybind11.hpp"

namespace rosbag2_py
{

template<typename T>
class Reader
{
public:
  Reader()
  : reader_(std::make_unique<rosbag2_cpp::Reader>(std::make_unique<T>()))
  {
  }

  void open(
    rosbag2_storage::StorageOptions & storage_options,
    rosbag2_cpp::ConverterOptions & converter_options)
  {
    reader_->open(storage_options, converter_options);
  }

  bool has_next()
  {
    return reader_->has_next();
  }

  /// Return a tuple containing the topic name, the serialized ROS message, and
  /// the timestamp.
  pybind11::tuple read_next()
  {
    const auto next = reader_->read_next();
    rcutils_uint8_array_t rcutils_data = *next->serialized_data.get();
    std::string serialized_data(rcutils_data.buffer,
      rcutils_data.buffer + rcutils_data.buffer_length);
    return pybind11::make_tuple(
      next->topic_name, pybind11::bytes(serialized_data), next->time_stamp);
  }

  /// Return a mapping from topic name to topic type.
  std::vector<rosbag2_storage::TopicMetadata> get_all_topics_and_types()
  {
    return reader_->get_all_topics_and_types();
  }

  void set_filter(const rosbag2_storage::StorageFilter & storage_filter)
  {
    return reader_->set_filter(storage_filter);
  }

  void reset_filter()
  {
    reader_->reset_filter();
  }

protected:
  std::unique_ptr<rosbag2_cpp::Reader> reader_;
};

std::unordered_set<std::string> get_registered_readers()
{
  std::unordered_set<std::string> combined_plugins = rosbag2_cpp::plugins::get_class_plugins
    <rosbag2_storage::storage_interfaces::ReadWriteInterface>();

  std::unordered_set<std::string> read_only_plugins = rosbag2_cpp::plugins::get_class_plugins
    <rosbag2_storage::storage_interfaces::ReadOnlyInterface>();

  // Merge read/write and read-only plugin sets
  combined_plugins.insert(read_only_plugins.begin(), read_only_plugins.end());

  return combined_plugins;
}

}  // namespace rosbag2_py

PYBIND11_MODULE(_reader, m) {
  m.doc() = "Python wrapper of the rosbag2_cpp reader API";

  pybind11::class_<rosbag2_py::Reader<rosbag2_cpp::readers::SequentialReader>>(
    m, "SequentialReader")
  .def(pybind11::init())
  .def("open", &rosbag2_py::Reader<rosbag2_cpp::readers::SequentialReader>::open)
  .def("read_next", &rosbag2_py::Reader<rosbag2_cpp::readers::SequentialReader>::read_next)
  .def("has_next", &rosbag2_py::Reader<rosbag2_cpp::readers::SequentialReader>::has_next)
  .def(
    "get_all_topics_and_types",
    &rosbag2_py::Reader<rosbag2_cpp::readers::SequentialReader>::get_all_topics_and_types)
  .def("set_filter", &rosbag2_py::Reader<rosbag2_cpp::readers::SequentialReader>::set_filter)
  .def("reset_filter", &rosbag2_py::Reader<rosbag2_cpp::readers::SequentialReader>::reset_filter);

  pybind11::class_<rosbag2_py::Reader<rosbag2_compression::SequentialCompressionReader>>(
    m, "SequentialCompressionReader")
  .def(pybind11::init())
  .def("open", &rosbag2_py::Reader<rosbag2_compression::SequentialCompressionReader>::open)
  .def(
    "read_next", &rosbag2_py::Reader<rosbag2_compression::SequentialCompressionReader>::read_next)
  .def("has_next", &rosbag2_py::Reader<rosbag2_compression::SequentialCompressionReader>::has_next)
  .def(
    "get_all_topics_and_types",
    &rosbag2_py::Reader<
      rosbag2_compression::SequentialCompressionReader
    >::get_all_topics_and_types)
  .def(
    "set_filter",
    &rosbag2_py::Reader<rosbag2_compression::SequentialCompressionReader>::set_filter)
  .def(
    "reset_filter",
    &rosbag2_py::Reader<rosbag2_compression::SequentialCompressionReader>::reset_filter);

  m.def(
    "get_registered_readers",
    &rosbag2_py::get_registered_readers,
    "Returns list of discovered plugins that support rosbag2 playback.");
}
