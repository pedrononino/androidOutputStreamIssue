void MainComponent::loadAudioButtonClicked()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Selecione...", juce::File(), "*.wav");

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
                             juce::FileBrowserComponent::canSelectFiles,
                             [this] (const juce::FileChooser& fileChooser)mutable
                             {

                                 if (!fileChooser.getURLResult().isEmpty()) {

                                     DBG('Got URL Result');
                                     auto bufferToWriteTo = writeBuffer.load();
                                     auto audioFileURL = fileChooser.getURLResult();
                                     auto androidDocument = juce::AndroidDocument::fromDocument(
                                             juce::URL(audioFileURL));
                                     auto *reader = formatManager.createReaderFor(
                                             androidDocument.createInputStream());
                                     bufferToWriteTo->setSize(reader->numChannels,
                                                              reader->lengthInSamples, false, true);
                                     backingTrackCursor = std::numeric_limits<int>::max();
                                     reader->read(bufferToWriteTo, 0, reader->lengthInSamples, 0,
                                                  true, true);

                                     DBG("BackingTrack length");
                                     DBG(reader->lengthInSamples);
                                     DBG("BackingTrack channels");
                                     DBG(int(reader->numChannels));
                                     DBG("BackingTrack sampleRate");
                                     DBG(reader->sampleRate);

                                     delete reader;

                                     reader = nullptr;

                                     writeBuffer.store(readBuffer.load());
                                     readBuffer.store(bufferToWriteTo);
                                 }
                             });


}

void MainComponent::saveMusicButtonClicked()
{
    auto backingTrackSavingBuffer = readBuffer.load();
    int backingTrackSize = backingTrackSavingBuffer->getNumSamples();

    auto voiceSavingBuffer = voiceReadBuffer.load();
    auto voiceSize = voiceSavingBuffer->getNumSamples();

    int max_num = std::max(voiceSize, backingTrackSize);


    backingTrackSavingBuffer->setSize(1, max_num, true);
    voiceSavingBuffer->setSize(1, max_num, true);
    mixedBuffer.setSize(2, max_num, false, true);

    auto channelData = mixedBuffer.getWritePointer(0);

    DBG("backingTrackSize: ");
    DBG(backingTrackSize);
    DBG("voiceSize: ");
    DBG(voiceSize);
    DBG("Max num: ");
    DBG(max_num);


    for (int currentSample = 0; currentSample < max_num; currentSample++) {

        float backingtrackBufferFloat = (backingTrackSavingBuffer->getSample(0, currentSample)) * 0.5;

        float voiceBufferFloat = (voiceSavingBuffer->getSample(0, currentSample)) * 0.5;

        channelData[0] = backingtrackBufferFloat + voiceBufferFloat;
        channelData[1] = backingtrackBufferFloat + voiceBufferFloat;

        mixedBuffer.addSample(0, currentSample, channelData[0]);
        mixedBuffer.addSample(1, currentSample, channelData[1]);


    }

    const auto flags = juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::warnAboutOverwriting;
    chooser.launchAsync(flags, [mixedBuffer](const juce::FileChooser& c)
        {
            const auto wavBlock = [&]
            {
                juce::MemoryBlock block;
                auto stream = std::make_unique<juce::MemoryOutputStream>(block, false);

                const auto writer = rawToUniquePtr(juce::WavAudioFormat().createWriterFor(stream.get(), 44100, 1, 32, {}, 0));

                if (!writer)
                {
                    jassertfalse;
                    return block;
                }

                stream.release();
                writer->writeFromAudioSampleBuffer(mixedBuffer, 0, mixedBuffer.getNumSamples());
                return block;
            }();

            auto doc = juce::AndroidDocument::fromDocument(c.getURLResult());

            if (!doc)
            {
                jassertfalse;
                return;
            }

            auto androidOutputStream = doc.createOutputStream();

            if (!androidOutputStream)
            {
                jassertfalse;
                return;
            }

            androidOutputStream->write(wavBlock.getData(), wavBlock.getSize());
        });



}
