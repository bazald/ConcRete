#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h>
#endif

#include "Zeni/Concurrency/Worker_Threads.hpp"
#include "Zeni/Rete/Network.hpp"
#include "Zeni/Rete/Parser.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //_CrtSetBreakAlloc(4479);
#endif

  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::Normal, nullptr);
  const auto parser = Zeni::Rete::Parser::Create();

  while (!network->get()->is_exit_requested() && std::cin) {
    std::cout << "ConcRete$ ";
    std::string command;
    std::getline(std::cin, command);

    std::cerr << "Processing: " << command << std::endl;

    network->get()->set_worker_threads(Zeni::Concurrency::Worker_Threads::Create());

    parser->parse_string(network->get(), network->get()->get_Worker_Threads()->get_main_Job_Queue(), command, true);

    (*network)->finish_jobs_and_destroy_worker_threads();
  }

  network->get()->set_worker_threads(Zeni::Concurrency::Worker_Threads::Create());

  /// Extra pass needed for now to ensure that extra handles stored interally get cleared
  network->get()->excise_all(network->get()->get_Worker_Threads()->get_main_Job_Queue(), false);
  network->get()->finish_jobs();

  return 0;
}
