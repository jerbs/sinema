//
// Sinemad Interface
//
// Copyright (C) Joachim Erbs, 2010
//
//    This file is part of Sinema.
//
//    Sinema is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Sinema is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Sinema.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef SINEMAD_INTERFACE_HPP
#define SINEMAD_INTERFACE_HPP

#include "platform/interface.hpp"
#include "receiver/TunerFacade.hpp"

namespace sdif
{

struct SinemadInterface
{
    typedef boost::mpl::vector<
	itf::procedure<TunerOpen,        itf::none>,
	itf::procedure<TunerClose,       itf::none>,
	itf::procedure<TunerTuneChannel, itf::none>,
	itf::procedure<TunerStartScan,   itf::none>,
	itf::procedure<itf::none, TunerScanFinished>,
	itf::procedure<itf::none, TunerScanStopped>,
	itf::procedure<itf::none, TunerNotifySignalDetected>,
	itf::procedure<itf::none, TunerNotifyChannelTuned>
    > type;
};

}

#endif
