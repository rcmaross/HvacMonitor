#pragma once

#include <Arduino.h>
#include "Status.h"

class StatusWeb
{
public:
    StatusWeb(Status& status);

    String statusJson() const;

private:
    Status& _status;
};