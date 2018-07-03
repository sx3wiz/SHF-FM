#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <iostream>
#include <math.h>
#include <memory>
#include <thread>
#include <volk/volk.h>
#include <gnuradio/math.h>
#include <gnuradio/filter/fir_filter.h>
#include <gnuradio/filter/firdes.h>
//#include "json.hpp"
#include <fstream>
#include "argh.h"

#include "audio.hpp"

#define LOGURU_IMPLEMENTATION 1
#include "loguru.hpp"

//using json = nlohmann::json;
using namespace std;

constexpr double pi() { return 3.141592653589793238462643383279502884; }

int UHD_SAFE_MAIN(int argc, char *argv[])
{
  uhd::set_thread_priority_safe();

  loguru::init(argc, argv);
  loguru::add_file("shf-fm.log", loguru::Append, loguru::Verbosity_MAX);

  argh::parser cmdl(argv);

  double rate, center_freq, gain, bw;

  cmdl("rate", 1984500) >> rate;
  cmdl("center_freq", 5760.1e6) >> center_freq;
  cmdl("gain", 50) >> gain;
  cmdl("bw", 200000) >> bw;

  LOG_F(INFO, "Got rate: %f\n", rate);
  LOG_F(INFO, "Got center frequency: %f", center_freq);
  LOG_F(INFO, "Got gain: %f", gain);
  LOG_F(INFO, "Got bw: %f", bw);

  std::string device_args("type=b200");
  std::string ant("TX/RX");
  std::string ref("internal");

  uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(device_args);

  usrp->set_clock_source(ref);
  usrp->set_tx_rate(rate);
  cout << "Got tx rate: " << usrp->get_tx_rate() << endl;
  uhd::tune_request_t tune_request(center_freq);
  usrp->set_tx_freq(tune_request);
  cout << "Got center freq: " << usrp->get_tx_freq() << endl;
  usrp->set_tx_gain(gain);
  usrp->set_tx_bandwidth(bw);
  usrp->set_tx_antenna(ant);

  uhd::stream_args_t stream_args("fc32");
  uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

  uhd::tx_metadata_t meta;
  meta.end_of_burst = true;
  meta.has_time_spec = false;
  meta.start_of_burst = true;

  gr::filter::kernel::fir_filter_fff lp1(1,
    gr::filter::firdes::low_pass(1, int(rate), 200000, 50000));

  snd_pcm_t* audio_dev = get_alsa_pcm_handle("test");

  if ( audio_dev == NULL )
  {
    cerr << "Unable to open audio device" << endl;
    return 0;
  }

  float sensitivity = 1.0f;
  float phase = 0;
  float I, Q;
  int interp_rate = rate / 44100;

  while (1)
  {
    std::vector<float> audio_samps = read_audio_samples(audio_dev, 1024);
    std::vector<float> interped_samps(1024 * interp_rate, 0.0f);
    for(int i = 0; i < 1024 * interp_rate;i++)
    {
      interped_samps[i * interp_rate] = audio_samps[i];
    }

    std::vector<float> filtered_samps(1024 * interp_rate, 0.0f);

    lp1.filterN(&filtered_samps[0], &interped_samps[0], interped_samps.size());

    std::vector<std::complex<float>> mod_samps(1024 * interp_rate);

    for(int i = 0;i < 1024;i++)
    {
      float dev = filtered_samps[i];
      phase = phase + sensitivity * dev;
      // Keep phase within bounds
      phase = std::fmod(phase + (float)M_PI, 2.0f * (float)M_PI) - (float)M_PI;

      I = sin(phase);
      Q = cos(phase);

      mod_samps[i] = std::complex<float>(I, Q);
    }

    tx_stream->send(&mod_samps[0], mod_samps.size(), meta, 1);
  }

  close_alsa_handle(audio_dev);

  return EXIT_SUCCESS;
}
