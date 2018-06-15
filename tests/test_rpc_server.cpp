#include "test_rpc_server.h"
#include "logic/process_reader.h"
#include "test.h"

#include <grpc++/grpc++.h>
#include <iterator>
#include <memory>
#include <sce.grpc.pb.h>
#include <sce.pb.h>
#include <sstream>
#include <string>
#include <thread>

struct Test_RPC_server {
	static constexpr auto test_response = "testresponse";
	static constexpr auto rpc_address = "localhost:53676";

	//class that implements some RPC functions
	class Rpc_server : public sce::proto::Query::Service {
		grpc::Status Test([[maybe_unused]] grpc::ServerContext *context, [[maybe_unused]] const sce::proto::TestIn *request,
						  sce::proto::TestOut *response) override {
			response->set_message(test_response);
			return grpc::Status::OK;
		}
	};

	Test_RPC_server()
		//create RPC server and run it in a thread
		: server{grpc::ServerBuilder{}
					 .SetDefaultCompressionLevel(GRPC_COMPRESS_LEVEL_NONE)
					 .SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE)
					 .AddListeningPort(rpc_address, grpc::InsecureServerCredentials())
					 .RegisterService(&rpc_server)
					 .BuildAndStart()}
		, server_thread{[this] { server->Wait(); }} {}
	~Test_RPC_server() {
		server->Shutdown();
		server_thread.join();
	}

	Rpc_server rpc_server;
	decltype(grpc::ServerBuilder{}.BuildAndStart()) server;
	std::thread server_thread;
};

static void test_local_rpc_call() {
	Test_RPC_server rpc_server;
	//make an RPC call
	sce::proto::TestOut reply;
	grpc::ClientContext client_context;
	auto stub = sce::proto::Query::NewStub(grpc::CreateChannel(Test_RPC_server::rpc_address, grpc::InsecureChannelCredentials()));
	auto status = stub->Test(&client_context, {}, &reply);
	assert_true(status.ok());
	assert_equal(reply.message(), Test_RPC_server::test_response);
}

static void test_python_rpc_call() {
	Test_RPC_server trpcs;

	auto test_sh_script = [](QString script, std::string_view expected_output, std::string_view expected_error) {
		std::ostringstream output, error;
		Process_reader::run("sh", std::move(script), output, error);
		assert_equal(expected_error, error.str());
		assert_equal(expected_output, output.str());
	};

	//python2
	test_sh_script(R"(run_python_script.sh "python2 rpc_call.py")", "testresponse", "");
	//python3
	test_sh_script(R"(run_python_script.sh "python3 rpc_call.py")", "testresponse", "");
}

void test_rpc_server() {
	test_local_rpc_call();
	test_python_rpc_call();
}
