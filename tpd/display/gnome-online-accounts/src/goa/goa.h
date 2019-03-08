/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __GOA_H__
#define __GOA_H__

#if !defined(GOA_API_IS_SUBJECT_TO_CHANGE) && !defined(GOA_COMPILATION)
#error  libgoa is unstable API. You must define GOA_API_IS_SUBJECT_TO_CHANGE before including goa/goa.h
#endif

#define __GOA_INSIDE_GOA_H__
#include <goa/goatypes.h>
#include <goa/goaclient.h>
#include <goa/goaerror.h>
#include <goa/goa-generated.h>
#undef __GOA_INSIDE_GOA_H__

#endif /* __GOA_H__ */
