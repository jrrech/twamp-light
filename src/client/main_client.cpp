//
// Created by vladim0105 on 12/15/21.
//
#include <unistd.h>
#include "Client.h"
#include "CLI11.hpp"
Args parse_args(int argc, char **argv){
    Args args;
    args.payload_lens.push_back(50);
    args.payload_lens.push_back(100);
    args.payload_lens.push_back(150);
    uint8_t dscp = 0, tos = 0;
    CLI::App app{"Twamp-Light implementation written by Domos."};
    app.option_defaults()->always_capture_default(true);
    app.add_option("address", args.remote_host, "The address of the remote TWAMP Server.");
    app.add_option("-a, --local_address", args.local_host, "The address to set up the local socket on. Auto-selects by default.");
    app.add_option("-P, --local_port", args.local_port, "The port to set up the local socket on.");
    app.add_option("-p, --port", args.remote_port, "The port that the remote server is listening on.");
    app.add_option<std::vector<uint16_t>>("-l, --payload_lens", args.payload_lens,
            "The payload length. Must be in range (42, 1473). Can be multiple values, in which case it is selected randomly.")
            ->default_str(vectorToString(args.payload_lens, " "))->check(CLI::Range(42, 1473));
    app.add_option("-n, --num_samples", args.num_samples, "Number of samples to expect.");
    app.add_option("-t, --timeout", args.timeout, "How long (in seconds) to keep the socket open, when no packets are incoming.")->default_str(std::to_string(args.timeout));
    app.add_option("-d, --delay", args.delay_millis, "How long (in millis) to wait between sending each packet.");
    app.add_option("-s, --seed", args.seed, "Seed for the RNG. 0 means random.");
    auto opt_tos = app.add_option("-T, --tos", tos, "The TOS value (<256).")->check(CLI::Range(256))->default_str(std::to_string(args.snd_tos));
    auto opt_dscp = app.add_option("-D, --dscp", dscp, "The DSCP value (<64).")->check(CLI::Range(64))->default_str(std::to_string(args.dscp_snd));
    opt_tos->excludes(opt_dscp);
    opt_dscp->excludes(opt_tos);
    try{
        app.parse(argc, argv);
    }catch(const CLI::ParseError &e) {
        std::exit((app).exit(e));
    }

    if(*opt_tos){
        args.snd_tos = tos - (((tos & 0x2) >> 1) & (tos & 0x1));
    }
    if(*opt_dscp){
        args.snd_tos = dscp << 2;
    }
    return args;
}
int main(int argc, char **argv) {
    Args args = parse_args(argc, argv);
    Client client = Client(args);
    uint16_t lost_packets = 0;
    for(int i = 0; i < args.num_samples; i++){
        size_t payload_len = *select_randomly(args.payload_lens.begin(), args.payload_lens.end(), args.seed);
        client.sendPacket(i, payload_len);
        bool response = client.awaitResponse(payload_len, lost_packets, args);
        if(!response){
            lost_packets++;
        }
        usleep(args.delay_millis*1000);
    }
}