PROTOBUF_ABSL_DEPS = absl_absl_check absl_absl_log absl_algorithm absl_base absl_bind_front absl_bits absl_btree absl_cleanup absl_cord absl_core_headers absl_debugging absl_die_if_null absl_dynamic_annotations absl_flags absl_flat_hash_map absl_flat_hash_set absl_function_ref absl_hash absl_layout absl_log_initialize absl_log_severity absl_memory absl_node_hash_map absl_node_hash_set absl_optional absl_span absl_status absl_statusor absl_strings absl_synchronization absl_time absl_type_traits absl_utility absl_variant
PROTOBUF_UTF8_RANGE_LINK_LIBS = -lutf8_validity

CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc absl_flags absl_flags_parse`
CXXFLAGS += -std=c++14
LDFLAGS += -L/usr/local/lib `pkg-config --libs --static protobuf grpc++ absl_flags absl_flags_parse $(PROTOBUF_ABSL_DEPS)`\
           $(PROTOBUF_UTF8_RANGE_LINK_LIBS) \
           -pthread\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = protos

PROTO_GEN_SRCS = src/helloworld.pb.cc src/helloworld.grpc.pb.cc
PROTO_GEN_HDRS = src/helloworld.pb.h src/helloworld.grpc.pb.h

vpath %.proto $(PROTOS_PATH)

all: greeter_client greeter_server

src/%.grpc.pb.cc: proto/%.proto
	$(PROTOC) $(PROTOCFLAGS) --grpc_out=./src --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<

src/%.pb.cc: proto/%.proto
	$(PROTOC) $(PROTOCFLAGS) --cpp_out=./src $<

src/server.o: src/server.cc $(PROTO_GEN_HDRS)
	$(CXX) $(CXXFLAGS) -c src/server.cc -o src/server.o

src/client.o: src/client.cc $(PROTO_GEN_HDRS)
	$(CXX) $(CXXFLAGS) -c src/client.cc -o src/client.o

greeter_client: src/helloworld.pb.o src/helloworld.grpc.pb.o src/greeter_client.o
	$(CXX) $^ $(LDFLAGS) -o $@

greeter_server: src/helloworld.pb.o src/helloworld.grpc.pb.o src/greeter_server.o
	$(CXX) $^ $(LDFLAGS) -o $@

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

clean:
	rm -f src/*.o src/*.pb.cc src/*.pb.h greeter_client greeter_server
