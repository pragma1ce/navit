
#include <gtest/gtest.h>

#include "mocks/navitprocessmock.h"
#include "mocks/navitcontrollermock.h"
#include "nxe_instance.h"
#include "settingtags.h"
#include "jsonmessage.h"
#include "../testutils.h"
#include "log.h"

using ::testing::StrictMock;
const std::string navitPath { NAVIT_PATH };

struct NavitInstanceTest : public ::testing::Test {

    std::shared_ptr<StrictMock<NavitProcessMock> > mock{ new StrictMock<NavitProcessMock>{} };
    std::shared_ptr<NXE::NavitProcess> process{ mock };
    std::shared_ptr<StrictMock<NavitControllerMock> > controllerMock { new StrictMock<NavitControllerMock>{} };
    std::shared_ptr<NXE::NavitController> controller { controllerMock };

    bool bData = false;

    static void SetUpTestCase()
    {
        TestUtils::createNXEConfFile();
    }

    void callback(const std::string &str) {
        ASSERT_NE(str, "error");
        nDebug() << "Callback data=" << str;
        bData = true;
    }
};

TEST_F(NavitInstanceTest, moveBy_without_data)
{
    using ::testing::Return;
    using ::testing::_;

    // Arrange
    NavitProcessMock* mock = dynamic_cast<NavitProcessMock*>(process.get());
    NavitControllerMock *cmock = dynamic_cast<NavitControllerMock*>(controller.get());
    EXPECT_CALL(*mock, isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*mock, start()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock, setProgramPath(navitPath));
    EXPECT_CALL(*mock, stop());
    EXPECT_CALL(*cmock, start());

    NXE::NXEInstance instance{ process, controller };
    instance.registerMessageCallback(std::bind(&NavitInstanceTest::callback, this, std::placeholders::_1));
    const std::string incomingMessage = "{\"id\":0, \"call\":\"moveBy\"}";
    EXPECT_NO_THROW(instance.HandleMessage(incomingMessage.data()));
    EXPECT_TRUE(bData);
}

TEST_F(NavitInstanceTest, moveBy_with_data)
{
    using ::testing::Return;
    using ::testing::_;

    // Arrange
    ASSERT_FALSE(bData);
    NavitProcessMock* mock = dynamic_cast<NavitProcessMock*>(process.get());
    NavitControllerMock *cmock = dynamic_cast<NavitControllerMock*>(controller.get());
    EXPECT_CALL(*mock, isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*mock, start()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock, setProgramPath(navitPath));
    EXPECT_CALL(*mock, stop());
    EXPECT_CALL(*cmock, start());
    EXPECT_CALL(*cmock, moveBy(-15,-15));

    NXE::NXEInstance instance{ process, controller };
    instance.registerMessageCallback(std::bind(&NavitInstanceTest::callback, this, std::placeholders::_1));
    const std::string incomingMessage =
            "{ \
                \"id\":0, \
                \"call\":\"moveBy\", \
                \"data\": { \
                    \"x\": -15,\
                    \"y\": -15 \
                } \
            }";
    EXPECT_NO_THROW(instance.HandleMessage(incomingMessage.data()));
    EXPECT_TRUE(bData);
}
