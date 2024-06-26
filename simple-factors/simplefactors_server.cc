#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "SimpleFactors.grpc.pb.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using simplefactor::Factorer;
using simplefactor::FactorReply;
using simplefactor::FactorRequest;

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");
ABSL_FLAG(std::string, log_file_name, "log.txt", "Name of log file");

ofstream log_file;

void log(string message) {

    try {
        if (!log_file.is_open()) {
            log_file.open(absl::GetFlag(FLAGS_log_file_name));
        }
        log_file << message;
    } catch (...) {
        std::exception_ptr ex = std::current_exception();
        try {
            log_file.close();
            std::rethrow_exception(ex);
        } catch (std::bad_exception const &) {
            // This will be printed.
            std::cout << "Bad exception" << std::endl;
        }
    }
}

class FactorerServiceImpl final : public Factorer::Service {
    Status Factor(ServerContext *context, const FactorRequest *request,
                  FactorReply *reply) override {
        int target = request->target();
        try {
            primeFactors(target, reply);
            return Status::OK;
        } catch (...) {
            return Status(StatusCode::UNKNOWN,"Unable to factor " + to_string(target));
        }
    }

    void primeFactors(int n, FactorReply *reply) {
        log("Factor " + to_string(n) + ": ");
        while (n % 2 == 0) {
            log("2 ");
            reply->add_factors(2);
            n = n / 2;
        }

        for (int i = 3; i <= sqrt(n); i = i + 2) {
            while (n % i == 0) {
                log(to_string(i) + " ");
                reply->add_factors(i);
                n = n / i;
            }
        }
        if (n > 2) {
            log(to_string(n) + "\n");
            reply->add_factors(n);
        } else
            log("\n");
    }
};

void RunServer(uint16_t port) {
    std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
    FactorerServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char **argv) {
    absl::ParseCommandLine(argc, argv);
    log_file.exceptions(std::ios_base::failbit);
    cout << "Logging BBB to " << absl::GetFlag(FLAGS_log_file_name) << std::endl;
    RunServer(absl::GetFlag(FLAGS_port));
    return 0;
}
