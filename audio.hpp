#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <iostream>

using namespace std;

snd_pcm_t* get_alsa_pcm_handle(std::string dev)
{
  int err;
  snd_pcm_t* capture_handle;
  snd_pcm_hw_params_t* hw_params;

  if ((err = snd_pcm_open (&capture_handle, dev.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n",
       dev.c_str(),
       snd_strerror (err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
    fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
       snd_strerror (err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
       snd_strerror (err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "cannot set access type (%s)\n",
       snd_strerror (err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
    fprintf (stderr, "cannot set sample format (%s)\n",
       snd_strerror (err));
    return NULL;
  }

  unsigned int rate = 44100;
  int dir = 0;

  if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, &dir)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n",
       snd_strerror (err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
       snd_strerror (err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
    fprintf (stderr, "cannot set parameters (%s)\n",
       snd_strerror (err));
    return NULL;
  }

  snd_pcm_hw_params_free (hw_params);

  if ((err = snd_pcm_prepare (capture_handle)) < 0) {
    fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
       snd_strerror (err));
    return NULL;
  }

  return capture_handle;
}

std::vector<float> read_audio_samples(snd_pcm_t* handle, int num)
{
  std::vector<short> samples(num);

  int i = 0;
  while ( i < num )
  {
    int res = snd_pcm_readi(handle, &samples[i], (num - i));
    if ( res < 0 )
    {
      cerr << "Erro reading ALSA samples!" << endl;
      return std::vector<float>(0);
    }
    i += res;
  }

  std::vector<float> fsamples(samples.size());
  for(int i = 0;i < samples.size();i++)
  {
    fsamples[i] = static_cast<float>(samples[i]) / 32768.0f;
  }

  return fsamples;
}

void close_alsa_handle(snd_pcm_t* handle)
{
  snd_pcm_close(handle);
}
