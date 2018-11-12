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

static void test_Antiable_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Positive_Hash_Trie(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Rete_Network(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);
static void test_Parser(const std::shared_ptr<Zeni::Concurrency::Worker_Threads> &worker_threads, const std::shared_ptr<Zeni::Concurrency::Job_Queue> &job_queue);

static int16_t g_num_cores = 0;

int main(int argc, char **argv)
{
#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //_CrtSetBreakAlloc(4479);
#endif

  const auto network = Zeni::Rete::Network::Create(Zeni::Rete::Network::Printed_Output::Normal, nullptr);
  const auto parser = Zeni::Rete::Parser::Create();

  while(std::cin) {
    std::cout << "ConcRete$ ";
    std::string command;
    std::getline(std::cin, command);

    std::cerr << "Processing: " << command << std::endl;

    network->get()->set_worker_threads(Zeni::Concurrency::Worker_Threads::Create());

    parser->parse_string(network->get(), network->get()->get_Worker_Threads()->get_main_Job_Queue(), command, true);

    (*network)->finish_jobs_and_destroy_worker_threads();
  }

  return 0;
}
