#set page(
	fill: rgb("283238")
)

#set text(
	fill: white,
	font: "Helvetica",
	size: 14pt
)

#align(center + top)[
	#image(
		"./arboreal tree.svg",
		width: 13%,
		fit: "cover"
	)
]

#set align(center)
Arboreal Audio presents:

#set text(
	font: "Sora",
	size: 16pt
)

= PiMax
== Loudness Maximizer & Multiband Enhancer

#linebreak()

#align(center)[
	#image(
		"UI_full.png",
		width: 100%
	)
]
#linebreak()

#set align(left)
#set text(
	font: "Helvetica",
	size: 12pt
)

== What is PiMax?

Well hopefully you're not asking that after _buying_ it.

But regardless, PiMax is what's commonly referred to as a "loudness maximizer", which is another way of saying it's a soft clipper with an emphasis on mostly transparent or subtle saturation, allowing you to get more volume and perceived loudness out of any given sound source.

This happens by distorting or clipping the sound source when it reaches a certain level. The reason you often hear "clipping" discussed in a negative context is because that usually refers to "hard clipping". Hard clipping occurs when there's no gradual transition into the clipping threshold, the audio is either clean or it gets clipped. At low levels, this may be inaudible and won't really matter. But at a certain point, it creates unpleasant crackling or popping sounds, or even more gross distortion effects. Now, if that sounds good to you, that's fine, but generally audio engineers strive to avoid hard clipping. But they _love_ soft clipping.

#columns(2)[
=== Hard Clipping:
#align(left)[
	#image("hardclip.png")
]
#colbreak()
=== Soft Clipping:
#align(right)[
	#image("softclip.png")
]
]

These graphs illustrate what happens to the sound as it is amplified into the clipper, where the X-axis is the input signal's amplitude (positive or negative), and the Y-axis is the output signal's amplitude. You can envision the purple and green lines as showing the gain applied to the signal as it increases.

Or, imagine the oscillating waveforms of a sound source, and imagine that the horizonal lines across 1 and -1 are the limits of both clippers.

In the case of the hard clipper, if the peaks of the waveform go past 1 or -1, they get cut off sharply. But in the case of the soft clipper, the clipping slowly kicks in. This more "natural" shape of the soft clipper more closely resembles what analog distortion is like. And having just a small amount of clipping applied to your signal can sound pleasing; it can make things sound more dense by reducing the difference between peaks and the rest of the sound, and it can do this well on any sound source you put through it. Soft clipping on drums works great in tandem with compression to make them sound more dense and punchy, and it can also make a full mix sound louder and denser by shaving off the peaks. But please---clip responsibly.

If driven hard enough, a soft clipper can become basically a hard clipper! Maybe that's what you want?!?

There's a lot more we could get into with the science behind distorting audio in a way that sounds good, but that's a can of worms probably too great for this humble little manual. Suffice to say, PiMax contains numerous features to allow you to sculpt the distortion to fit whatever your current need is.

#pagebreak()

== The Controls: Full Band

=== Input

PiMax has an input gain control, which is where the bulk of dialing in a good sound will happen. The more gain = the more distortion!

=== Output

The output gain controls the outgoing volume after all other effects (including Auto gain). This is where you'd fine-tune the output of PiMax to fit in with your gain staging or to mix will with the rest of your tracks.

This control _does not_ affect the saturation of PiMax. It is a purely linear volume control. Although you do see the waveshaper graph shrinking or expanding when this is adjusted, that is because it is scaling the amplitude of the output --- just clean, linear multiplication.

=== Saturation Types

There are an array of controls available to adjust the way PiMax saturates:

==== Symmetric/Asymmetric

"Symmetry" in saturation describes how the clipper behaves with respect to the positive and negative halves of the sonic waveform. In Symmetric mode, both positive and negative parts of the waveform are saturated. This is pretty standard behavior for a soft-clipper, and will produce only odd-ordered harmonics. In Asymmetric mode, you get a different level of saturation for positive and negative halves, and this produces both even- and odd-order harmonics. Lots of analog gear produces asymmetric saturation, and the extra harmonics add a warm touch.

==== Clip Mode

There are 5 (as of PiMax v1.1) clipping modes in PiMax that drastically change how the saturation behaves.

===== Finite

This combines soft clipping with wavefolding to produce a unique type of saturation, where peaks above the clipping threshold start folding back towards zero. This can affect the sound in ways that most soft clippers don't, and also produces less harmonics than some of the other modes.

===== Clip

A pure soft clipper, this can be useful instead of Finite mode if you're saturating transient-heavy material. The threshold of 0dB means that more energy will be preserved than in Finite or Infinite modes.

===== Infinite

The simplest (mathematically speaking) saturation mode, this applies theoretically infinite wavefolding to the signal. It's:

$ sin(pi/2 * x) $

if you were wondering. This produces the least amount of harmonics compared to any other mode, but the wavefolding effect can result in seeming changes in pitch to the signal. Using it at low levels is a great way to subtly add harmonics to a signal.

===== Deep

New in PiMax v1.1, this is an unapolagetically dirty saturation mode. It saturates at basically any input level, and it also imparts a slight boost to the low- and top-end. It can sound like a raunchy piece of analog equipment, or like a bizarre layer of noisy harmonics layered on top of your signal.

===== Warm

The subtlest mode in terms of saturation, but the most heavy-handed in terms of filtering, this mode absorbs high-end and saturates very gently. Probably the most "analog" mode, this is great for making sounds more vintage, warm, or distant-sounding. You can control the perceived warmth with the Curve control.

=== Curve

The Curve control allows further customization over the saturation response of PiMax.

In Finite, Clip, and Infinite modes, there are two different saturation algorithms for the positive and negative sides of the slider. Both of them are "stateful" saturation modes which behave a bit more like analog distortion---without getting into graphic detail, this mostly has to do with the fact that the output depends on not only the input, but the previous inputs as well, adding a more dynamic element to the sound.

On the positive (green) side, you'll get a warmer, more compressed saturation.

On the negative (blue) side, you'll get a saturation curve that amplifies loud signals but attenuates queiter signals. This can sound like dynamic expansion---but it's also got noisy, unsubtle harmonics to go with it.

In Deep mode, this controls the clipping threshold, where positive values will raise it and give a harder "knee" to the clipping, and this is probably the cleanest you can get Deep mode---while negative values will open up the threshold and dig in harder at lower levels.

In Warm mode, this controls the frequency response of the saturation. Going positive will absorb more high-end and give a nice low-end bump, and going negative will reduce the high-frequency cut, though not fully.

#pagebreak()

== Waveshaper Display

#image(
	"waveshaper.png",
)

For the visually-inclined, the display in the center of the UI shows you how PiMax is clipping the signal. It's also affected by the output gain, but remember that the output gain does not affect the amount of saturation applied, only the outgoing volume.

== The Controls: Band Split

#image(
	"bandsplit.png"
)

By enabling Band Split, you can open up the versatility of PiMax even more, taking advantage of its modular multiband mode.

=== What is multiband?

Multiband is a method of splitting a signal into different frequency groups or "bands". By default PiMax splits into three bands: low, middle, and high frequencies. Then each band is saturated seperately. This is useful when you want more subtle saturation effects, or finer control over the frequency-dependence of the saturation. For instance, you may have a mix with a prominent low-end, and by saturating the full band all together, the bass causes too much saturation and disrupts the mix---but you really want to add some clipping to the midrange.

This is where multiband comes in. You can have full control over which frequencies get saturated and by how much.

=== Crossovers

The vertical handles across the display are the crossover points of the multiband filters. This controls the divide between seperate bands. So if you wanted to seperate the bass from the rest of the signal, place a crossover somewhere between 150-200Hz. The crossovers can go from 40Hz to 18,000Hz.

If you want to delete a crossover point, just right-click on the handle.

Additionally, you can add a new point by clicking the + button at the top of the display at the point on the spectrum that you wish.

=== Band Controls

Each band has its own set of controls:

==== Solo

This will isolate the signal within the band

==== Mute

Mutes an individual band

==== Bypass

This will bypass the saturation for an individual band

==== Input

The gain of the band going into the saturation. If Auto gain is on, this gain will be compensated for after the saturation.

==== Output

The gain of the band after the saturation. You can use this like a regular EQ gain.

==== Width

An extra width control for the stereo image of an individual band. This allows you to take your sonic sculpting to the next level, and works wonders on polishing a full mix. Use this to focus the bass towards the center of the stereo image, or to widen the cymbals of a drum kit, or give the midrange tone of a reverb more space.

If you enable Mono Width on the master Width control, this will switch the band widener to a mono->stereo widener (provided your channel configuration in your DAW allows for this). If you've got a mono guitar or vocal track, for instance, you can use this to create frequency-specific stereo wideness on a track that would otherwise be totally mono.

Additionally, if this is being used in a mono->stereo context, the master Width control will function like a regular width control---as in, it won't "synthesize" a stereo image of the full-band signal, but will simply widen the stereo image like normal.

#pagebreak()

== Extra Controls

#align(center)[
	#image("xtra_controls.png")
]

It's the Current Year, and all your digital audio effects should have the following options:

=== HQ

4x, minimum-phase oversampling. This will reduce unpleasant digital artifacts from heavy distortion, especially on high-frequency content, at the expense of extra CPU use, a few samples of latency, and some phase shift at the very highest frequencies.

=== Render HQ

If the CPU use of HQ is too great for your modest computer you cobbled together out of potatoes and a couple batteries you found in a dumpster, consider giving Render HQ a try. It will enable (linear-phase) oversampling only when you're rendering your final project, saving you some CPU when mixing live, though it might increase rendering times if you've got many instances of PiMax.

=== Linear Phase

This will enable linear phase for all multiband filters and oversampling filters. This is primarily useful if you want to use the Mix knob in conjuction with HQ or Band Split and you don't want the phase cancellation issues that would occur otherwise. It will guarantee a fully linear, flat frequency response from the crossover and oversampling filters, at the expense of latency and increased CPU.

If you're using Band Split, don't mix with this off and then enable it before a render, because the presence or absence of phase shifts in the multiband filters will affect the sound in ways you might not realize. In general, it's probably best to use this in a mastering context, where latency isn't an issue, and where you're probably not running 64 tracks of Totally the Same Channel Strip that the Beatles Used™.

=== Width

The master stereo width control for PiMax. This will narrow or widen the stereo image, which is a great way to make stereo signal sound bigger, wider, or more enveloping.

==== Mono->Stereo

If you Alt/Option-click this knob, you can enable Mono Width mode, which can turn a mono signal into a stereo one. This works great for making a central vocal part feel like it's taking up more space, or can allow you to widen a mono guitar or bass track.

You don't have to worry about mono compatibility, either. The "synthesized" stereo signal is only added to the sides, so you won't get phase artifacts when playing back your track in mono.

If you use Mono Width in conjunction with the Band Split wideners, the individual band wideners will work in mono->stereo mode, and this knob will resume its role as a global stereo width control.

=== Mix

A dry/wet mixing knob for parallel processing. This can be a great way to reign in a heavy-handed saturation setting, and parallel saturation can add thickness while preserving some of the inherent dynamics of a sound. Keep in mind the Linear Phase option if you're using Band Split or HQ with this. Or, you can exploit the phase nonlinearities of the multiband filters to get some interesting sounds.

This is applied _after_ the Output gain control, since you will want to use the Output gain to fine-tune the volume compensation, and then use the Mix control to balance dry and wet signals.

==== Delta

If you Alt/Option-click the Mix knob, you enable Delta mode, which isolates all the processing that PiMax is applying to the signal. So, if you were curious to hear what saturation sounds like on its own, or how the multiband is affecting your signal, you can use this. Or, you can just leave it on for some weird, glitchy effects.

=== Bypass

A latency-compensated bypass switch, for comparing your signal against the dry, unprocessed signal.

=== Auto

An automatic gain compensation feature, which allows you to dial in saturation without getting huge changes in volume. This is especially useful in keeping yourself from getting overzealous with the saturation, because after a certain point you will notice that it begins to eat into your signal too much, which you otherwise may not have noticed if the signal weren't gain-compensated.

This also works for individual band gains, so you can drive each signal band without getting a jump in volume.

=== Input Boost

A +12dB gain boost. While this isn't a _necessary_ feature in most plugins, it can be useful here, if you're sending a quiet signal into PiMax, but you want to apply some healthy saturation without messing up your gain structure too much. This will also be compensated by Auto gain mode.

You can also just destroy your sound with this, and that's your right.

For added sonic chaos, the Curve paremeter will be raised x#super[2], i.e. squared, i.e. *more curve*---in Finite, Clip, and Infinte modes only. Watch out, it can get pretty nasty.

== Menu

The menu in the lower-left corner of the UI gives you a few useful options:

- Default UI size

- #text(fill: yellow)[( New in v1.1.3 )] Alt. Asym. Type --- A different type of asymmetric saturation that does not apply a static DC offset, but uses some asymmetric multiplication to bias the saturation. It's not really a big deal.

- Show Tooltips

*Only Windows/Linux:*

- OpenGL on/off

== Uninstallation

If you’ve had enough of our goofy plugin and want to be rid of it for good, you can delete the plugin at its installation location:

=== Windows

On Windows, an uninstaller should be created, and so you can remove the plugin via the Add or Remove Programs menu. If you can't find that, you can manually remove it at the following locations:

C:\\Program Files\\Common Files\\VST3

C:\\Program Files\\Steinberg\\VSTPlugins

=== Mac

Home/Library/Audio/Plug-Ins – then locate and eliminate PiMax in the VST3, VST, and Components folders

Additionally, the user presets folder can be found and destroyed at this location:

=== Windows

C:\\Program Files\\Users\\Username\\AppData\\Roaming\\Arboreal Audio\\PiMax

=== Mac

Home/Library/Arboreal Audio/PiMax

#pagebreak()
== Credits and Acknowledgements

Programming, DSP, and design: Alex Riccio

The development of PiMax was made possible with these open-source libraries:

#show link: underline

=== JUCE #super[1]
(c) 2022 Raw Material Software Limited

https://github.com/juce-framework/juce.git

=== VST3 SDK #super[1]
(c) 2024 Steinberg Media Group GmbH, All Rights Reserved

https://github.com/steinbergmedia/vst3sdk.git

=== PFFFT #super[2]
(c) 2020 Dario Mambro / (c) 2019 Hayati Ayguen / (c) 2013 Julien Pommier / (c) 2004 the University Corporation for Atmospheric Research (UCAR)

https://github.com/marton78/pffft.git

=== Gin #super[2]
(c) 2018, Roland Rabien

https://github.com/FigBug/Gin.git | https://www.rabiensoftware.com

=== CLAP / clap-juce-extensions #super[3]
(c) 2021 Alexandre BIQUE / (c) 2019-2020 Paul Walker

https://github.com/free-audio/clap-juce-extensions.git

#line(length: 100%, stroke: 1pt + white)

=== #super[1] GPLv3 License
https://www.gnu.org/licenses/gpl-3.0.en.html

=== #super[2] BSD-3-Clause License

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=== #super[3] MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
