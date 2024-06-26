#include <grpcpp/grpcpp.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "SimpleFactors.grpc.pb.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

using namespace std;
using namespace std::chrono;

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(int64_t, tries, 1, "Number of factoring tries");
ABSL_FLAG(bool, quiet, false, "Should results be sent to stdout");
ABSL_FLAG(bool, time, false, "Should we time the run");
ABSL_FLAG(bool, forever, false, "Should we loop forever");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using simplefactor::Factorer;
using simplefactor::FactorReply;
using simplefactor::FactorRequest;

class FactorClient {
   public:
    FactorClient(std::shared_ptr<Channel> channel)
        : stub_(Factorer::NewStub(channel)) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    int Factor(const int target, const bool quiet) {
        if (!quiet) cout << "Factor " << target << ": ";

        // Data we are sending to the server.
        FactorRequest request;
        request.set_target(target);

        // Container for the data we expect from the server.
        FactorReply reply;

        // Context for the client. It could be used to convey extra information
        // to the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->Factor(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            int num_factors = reply.factors_size();
            if (!quiet) {
                for (int i = 0; i < num_factors; i++) {
                    cout << reply.factors(i) << " ";
                }
                cout << std::endl;
            }
            return num_factors;
        } else {
            return -1;
        }
    }

   private:
    std::unique_ptr<Factorer::Stub> stub_;
};

int main(int argc, char **argv) {
    absl::ParseCommandLine(argc, argv);
    // Instantiate the client. It requires a channel, out of which the actual
    // RPCs are created. This channel models a connection to an endpoint
    // specified by the argument "--target=" which is the only expected
    // argument.
    std::string target_str = absl::GetFlag(FLAGS_target);
    // We indicate that the channel isn't authenticated (use of
    // InsecureChannelCredentials()).
    FactorClient factorer(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    long tries = absl::GetFlag(FLAGS_tries);
    bool time = absl::GetFlag(FLAGS_time);

    do {
        high_resolution_clock::time_point start = high_resolution_clock::now();

        for (int i = 0; i < tries; i++) {
            int reply = factorer.Factor(rand(), absl::GetFlag(FLAGS_quiet));
        }

        if (time) {
            high_resolution_clock::time_point stop =
                high_resolution_clock::now();
            duration<double, std::milli> timeRequired = (stop - start);
            cout << "Duration for " << tries
                 << " runs = " << timeRequired.count() << "ms" << std::endl;
        }
    } while (absl::GetFlag(FLAGS_forever));

    return 0;
}
